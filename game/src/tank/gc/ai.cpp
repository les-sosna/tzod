// ai.cpp: implementation of the GC_PlayerAI class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "ai.h"

#include "core/JobManager.h"
#include "core/Debug.h"

#include "fs/SaveFile.h"

#include "Macros.h"
#include "Functions.h"

#include "Vehicle.h"
#include "Turrets.h"
#include "Pickup.h"
#include "Player.h"
#include "Weapons.h"

#include "Particles.h"


///////////////////////////////////////////////////////////////////////////////
// Catmull-Rom interpolation

static void CatmullRom( const vec2d &p1, const vec2d &p2, const vec2d &p3, const vec2d &p4,
                        vec2d &out, float s )
{
	float s2 = s * s;
	float s3 = s2 * s;
	out = (p1*(2*s2-s3-s) + p2*(3*s3-5*s2+2) + p3*(4*s2-3*s3+s) + p4*(s3-s2)) * 0.5f;
}

///////////////////////////////////////////////////////////////////////////////

#define NODE_RADIUS         32.0f
#define MIN_PATH_ANGLE       0.4f

#define GRID_ALIGN(x, sz)    ((x)-(x)/(sz)*(sz)<(sz)/2)?((x)/(sz)):((x)/(sz)+1)

///////////////////////////////////////////////////////////////////////////////

JobManager<GC_PlayerAI> GC_PlayerAI::_jobManager;

IMPLEMENT_SELF_REGISTRATION(GC_PlayerAI)
{
	ED_SERVICE("ai", "AI");
	return true;
}

GC_PlayerAI::GC_PlayerAI()
{
	_desired_offset = 0;
	_current_offset = 0;

	_level = 2;

	_favoriteWeaponType = INVALID_OBJECT_TYPE;

	SetL2(L2_PATH_SELECT);
	SetL1(L1_NONE);
}

GC_PlayerAI::GC_PlayerAI(FromFile) : GC_Player(FromFile())
{
}

GC_PlayerAI::~GC_PlayerAI()
{
}

void GC_PlayerAI::Kill()
{
	if( !IsVehicleDead() )
	{
		_jobManager.UnregisterMember(this);
	}
	GC_Player::Kill();
}

void GC_PlayerAI::Serialize(SaveFile &f)
{
	GC_Player::Serialize(f);

	f.Serialize(_level);
	f.Serialize(_aiState_l1);
	f.Serialize(_aiState_l2);
	f.Serialize(_current_offset);
	f.Serialize(_desired_offset);
	f.Serialize(_favoriteWeaponType);
	f.Serialize(_pickupCurrent);
	f.Serialize(_target);

	if( f.loading() )
	{
		size_t size;
		f.Serialize(size);
		while( size-- )
		{
			_attackList.push_back(AttackListType::value_type());
			f.Serialize(_attackList.back());
		}

		if( !IsVehicleDead() )
		{
			_jobManager.RegisterMember(this);
		}
	}
	else
	{
		size_t size = _attackList.size();
		f.Serialize(size);
		for( AttackListType::const_iterator it = _attackList.begin(); _attackList.end() != it; ++it )
		{
			f.Serialize(*it);
		}
	}
}

void GC_PlayerAI::TimeStepFixed(float dt)
{
	GC_Player::TimeStepFixed(dt);

	if( IsVehicleDead() )
	{
		return;
	}


	VehicleState vs;
	ZeroMemory(&vs, sizeof(VehicleState));

	// clean the attack list
	struct helper{ static bool whether(const SafePtr<GC_RigidBodyStatic> &arg)
	{
		return arg->IsKilled();
	}};
	_attackList.remove_if(&helper::whether);


	if( _pickupCurrent )
	{
		if( _pickupCurrent->IsKilled() || !_pickupCurrent->IsVisible() )
		{
			_pickupCurrent = NULL;
		}
		else if( (_pickupCurrent->GetPos() - GetVehicle()->GetPos()).sqr() <
		          _pickupCurrent->GetRadius() * _pickupCurrent->GetRadius() )
		{
			vs._bState_AllowDrop = true;
		}
	}

	// получение настроек оружия
	AIWEAPSETTINGS weapSettings;
	if( GetVehicle()->GetWeapon() )
		GetVehicle()->GetWeapon()->SetupAI(&weapSettings);


	// принятие решения
	if( _jobManager.TakeJob(this) )
		SelectState(&weapSettings);


	// установка _current_offset для понижения меткости стрельбы
	const float acc_speed = 0.4f; // угловая скорость движения мнимой цели
	if( dynamic_cast<GC_Vehicle *>(GetRawPtr(_target)) )
	{
		float len = fabsf(_desired_offset - _current_offset);
		if( acc_speed*dt >= len )
		{
			_current_offset = _desired_offset;

			static float d_array[5] = {0.186f, 0.132f, 0.09f, 0.05f, 0.00f};

			float d = d_array[_level];

			if( _level > 2 )
			{
				d = d_array[_level] *
					fabsf(static_cast<GC_Vehicle*>(GetRawPtr(_target))->_lv.len()) /
						static_cast<GC_Vehicle*>(GetRawPtr(_target))->GetMaxSpeed();
			}

			_desired_offset = (d > 0) ? (g_level->net_frand(d) - d * 0.5f) : 0;
		}
		else
		{
			_current_offset += (_desired_offset - _current_offset) * dt * acc_speed / len;
		}
	}
	else
	{
		_desired_offset = 0;
		_current_offset = 0;
	}

	// исполнение принятого решения
	DoState(&vs, &weapSettings);


	//
	// управление фарами
	//
	switch( _level )
	{
	case 0:
	case 1:
		vs._bLight = true;
		break;
	case 2:
	case 3:
		vs._bLight = (NULL != _target);
		break;
	default:
		vs._bLight = false;
	}

	// send state to the vehicle
	GetVehicle()->SetState(vs);
}

bool GC_PlayerAI::CheckCell(const FieldCell &cell)
{
	if( (0xFF != cell.Properties() && GetVehicle()->GetWeapon()) ||
		(0 == cell.Properties() && !GetVehicle()->GetWeapon()) )
	{
		return true;
	}
	return false;
}

float GC_PlayerAI::CreatePath(float dst_x, float dst_y, float max_depth, bool bTest, const AIWEAPSETTINGS *ws)
{
	if( dst_x < 0 || dst_x >= g_level->_sx || dst_y < 0 || dst_y >= g_level->_sy )
	{
		return -1;
	}

	Field::NewSession();
	Field &field = g_level->_field;

	std::priority_queue<
		RefFieldCell, std::vector<RefFieldCell>, std::greater<RefFieldCell>
	> open;

	int start_x = GRID_ALIGN(int(GetVehicle()->GetPos().x), CELL_SIZE);
	int start_y = GRID_ALIGN(int(GetVehicle()->GetPos().y), CELL_SIZE);
	int end_x = GRID_ALIGN(int(dst_x), CELL_SIZE);
	int end_y = GRID_ALIGN(int(dst_y), CELL_SIZE);

	FieldCell &start = field(start_x, start_y);

	if( !CheckCell(start) ) return -1;

	start.Check();
	start.UpdatePath(end_x, end_y);
	start._pathBefore = 0;
	start._prevCell  = NULL;

	open.push( RefFieldCell(start) );


	while( !open.empty() )
	{
		FieldCell &cn = open.top();
		open.pop();

		if( cn.GetX() == end_x && cn.GetY() == end_y )
			break; // путь найден


		// порядок проверки соседних узлов
		//    4 | 0 | 6
		//   ---+---+---
		//    2 | n | 3
		//   ---+---+---
		//    7 | 1 | 5
		//                         0  1  2  3  4  5  6  7
		static int   per_x[8] = {  0, 0,-1, 1,-1, 1, 1,-1 };  // смещение клетки по x
		static int   per_y[8] = { -1, 1, 0, 0,-1, 1,-1, 1 };  // смещение клетки по y
		static float dist [8] = {
			1.0f, 1.0f, 1.0f, 1.0f,
			1.4142f, 1.4142f, 1.4142f, 1.4142f };             // стоимость пути

		// для проверки проходимости по диагонали
		//                           4     5     6     7
		static int check_diag[] = { 0,2,  1,3,  3,0,  2,1 };

		for( int i = 0; i < 8; ++i )
		{
			if( i > 3 ) // проверка проходимости по диагонали
			if( !CheckCell(field(cn.GetX() + per_x[check_diag[(i-4)*2  ]],
			                     cn.GetY() + per_y[check_diag[(i-4)*2  ]])) ||
			    !CheckCell(field(cn.GetX() + per_x[check_diag[(i-4)*2+1]],
			                     cn.GetY() + per_y[check_diag[(i-4)*2+1]])) )
			{
				continue;
			}


			FieldCell &next = field(cn.GetX() + per_x[i], cn.GetY() + per_y[i]);
			if( CheckCell(next) )
			{
				// увеличение стоимости пути при пробивании сквозь стены
				float dist_mult = 1;
				if( 1 == next.Properties() )
					dist_mult = ws->fDistanceMultipler;

				if( !next.IsChecked() )
				{
					next.UpdatePath(end_x, end_y);
					next.Check();
					next._pathBefore = cn._pathBefore + dist[i] * dist_mult;
					next._prevCell  = &cn;
					//-----------------
					if( next.Rate() < max_depth )
						open.push(RefFieldCell(next));
				}

				// next part of code causes assertions in <algorithm> because
				// it can modify cells that are being stored in "open" queue

				//else if( next._pathBefore > cn._pathBefore + dist[i] * dist_mult )
				//{
				//	next._pathBefore = cn._pathBefore + dist[i] * dist_mult;
				//	next._prevCell  = &cn;
				//	//-----------------
				//	if( next.Rate() < max_depth )
				//		open.push(RefFieldCell(next));
				//}
			}
		}
	}

	if( field(end_x, end_y).IsChecked() )
	{
		// путь найден
		const FieldCell *cell = &field(end_x, end_y);
		float distance = cell->_pathBefore;

		if( !bTest )
		{
			//
			// создание нового пути
			//

			ClearPath();

			PathNode node;

			node.coord.x = dst_x; node.coord.y = dst_y;
			_path.push_front(node);

			cell = cell->_prevCell;
			while( NULL != cell )
			{
				node.coord.x = (float) (cell->GetX() * CELL_SIZE);
				node.coord.y = (float) (cell->GetY() * CELL_SIZE);
				_path.push_front(node);

				for( int i = 0; i < cell->GetObjectsCount(); ++i )
				{
					_ASSERT(GetVehicle()->GetWeapon());
					_ASSERT(cell->Properties() > 0);

					GC_Actor *object = cell->GetObject(i);

					//
					// этот блок запрещает мочить свои стационарки.
					//  (не самая лучшая реализация)
					//
					if( GetTeam() && _level > 0 )
					{
						GC_Turret *pIsTurret = dynamic_cast<GC_Turret*>(object);
						if( pIsTurret && (pIsTurret->_team == GetTeam()) )
						{
							continue;
						}
					}
					_attackList.push_front(static_cast<GC_RigidBodyStatic*>(object));
				}

				cell = cell->_prevCell;
			}
		}

		return distance;
	}

	// путь не найден
	return -1;
}

void GC_PlayerAI::SmoothPath()
{
	if( _path.size() < 4 ) return;

	int init_size = _path.size();

	vec2d vn[4];
	std::list<PathNode>::iterator it[4], tmp;

	//
	// сглаживание углов
	//
	if( _path.size() > 4 )
	{
		it[1] = _path.begin();
		it[0] = it[1]++;
		while( it[1] != _path.end() )
		{
			PathNode new_node;
			new_node.coord = (it[0]->coord + it[1]->coord) * 0.5f;
			_ASSERT(new_node.coord.x > 0 && new_node.coord.y > 0);
			_path.insert(it[1], new_node);
			if( it[0] != _path.begin() )
				_path.erase(it[0]);
			it[0] = it[1]++;
		}
	}


	//
	// интерполяция сплайном
	//

	tmp = _path.begin();
	for( int i = 0; i < 4; ++i )
	{
		it[i] = tmp++;
		vn[i] = it[i]->coord;
		_ASSERT(vn[i].x > 0 && vn[i].y > 0);
	}

	for(;;)
	{
		PathNode new_node;

		for( int i = 1; i < 4; ++i )
		{
			CatmullRom(vn[0], vn[1], vn[2], vn[3], new_node.coord, (float) i / 4.0f);
			_ASSERT(new_node.coord.x > 0 && new_node.coord.y > 0);
			_path.insert(it[2], new_node);
		}

		for( int i = 0; i < 3; ++i )
		{
			it[i] = it[i+1];
			vn[i] = vn[i+1];
		}

		if( ++it[3] == _path.end() )
			break;

		vn[3] = it[3]->coord;
	}
}

std::list<GC_PlayerAI::PathNode>::const_iterator GC_PlayerAI::FindNearPathNode(const vec2d &pos) const
{
	_ASSERT(!_path.empty());
	std::list<PathNode>::const_iterator it = _path.begin(), result = it;
	float rr_min = (it->coord - pos).sqr();
	while( ++it != _path.end() )
	{
		float rr = (it->coord - pos).sqr();
		if( rr <= rr_min )
		{
			result = it;
			rr_min = rr;
		}
	}
	return result;
}

void GC_PlayerAI::ClearPath()
{
	_path.clear();
	_attackList.clear();
}

void GC_PlayerAI::RotateTo(VehicleState *pState, const vec2d &x, bool bForv, bool bBack)
{
	_ASSERT(!_isnan(x.x) && !_isnan(x.y));
	_ASSERT(_finite(x.x) && _finite(x.y));
	_ASSERT(x.x > 0 && x.y > 0);

	float ang2 = (x - GetVehicle()->GetPos()).Angle();
	float ang1 = GetVehicle()->_angle;

	float d1 = fabsf(ang2-ang1);
	float d2 = ang1 < ang2 ? ang1-ang2+PI2 : ang2-ang1+PI2;

	if( (d1 < MIN_PATH_ANGLE || d2 < MIN_PATH_ANGLE) && bForv )
	{
		pState->_bState_MoveForward = true;
	}

	if( (d1 < MIN_PATH_ANGLE || d2 < MIN_PATH_ANGLE) && bBack )
		pState->_bState_MoveBack = true;

	pState->_bExplicitBody = true;
	pState->_fBodyAngle = ang2;
}

void GC_PlayerAI::TowerTo(VehicleState *pState, const vec2d &x, bool bFire, const AIWEAPSETTINGS *ws)
{
	_ASSERT(GetVehicle());
	_ASSERT(GetVehicle()->GetWeapon());

	float ang2 = (x - GetVehicle()->GetPos()).Angle() + _current_offset;
	float ang1 = GetVehicle()->_angle + GetVehicle()->GetWeapon()->_angle;
	if( ang1 > PI2 ) ang1 -= PI2;


	float d1 = fabsf(ang2-ang1);
	float d2 = ang1 < ang2 ? ang1-ang2+PI2 : ang2-ang1+PI2;

	if( (d1 < ws->fMaxAttackAngle ||
		 d2 < ws->fMaxAttackAngle) && bFire)
	{
		pState->_bState_Fire = true;
	}

	pState->_bExplicitTower = true;
	pState->_fTowerAngle = ang2 - GetVehicle()->_angle - GetVehicle()->GetSpinup();
	//--------------------------------
	_ASSERT(!_isnan(pState->_fTowerAngle) && _finite(pState->_fTowerAngle));
}

// оценка полезности атаки данной цели
AIPRIORITY GC_PlayerAI::GetTargetRate(GC_Vehicle *target)
{
	_ASSERT(target);
	_ASSERT(GetVehicle());
	_ASSERT(GetVehicle()->GetWeapon());

	if( !target->GetPlayer() ||
		(0 != target->GetPlayer()->GetTeam() && target->GetPlayer()->GetTeam() == GetTeam()) )
	{
		return AIP_NOTREQUIRED; // своих и буздушных не атакуем
	}

	AIPRIORITY p = AIP_NORMAL;

	p += AIP_NORMAL * (GetVehicle()->GetHealth() / GetVehicle()->GetHealthMax());
	p -= AIP_NORMAL * (target->GetHealth() / target->GetHealthMax());

	return p;
}

// return TRUE еслт цель найдена
bool GC_PlayerAI::FindTarget(/*out*/ AIITEMINFO &info, const AIWEAPSETTINGS *ws)
{
	if( !GetVehicle()->GetWeapon() ) return FALSE;

	AIPRIORITY optimal = AIP_NOTREQUIRED;
	GC_Vehicle *pOptTarget = NULL;

	std::vector<TargetDesc> targets;

	//
	// проверяем цели
	//

	FOREACH( g_level->GetList(LIST_vehicles), GC_Vehicle, object )
	{
		if( !object->IsKilled() && object != GetVehicle() )
		{
			if( (GetVehicle()->GetPos() - object->GetPos()).sqr() <
				(AI_MAX_SIGHT * CELL_SIZE) * (AI_MAX_SIGHT * CELL_SIZE) )
			{
				GC_RigidBodyStatic *pObstacle = static_cast<GC_RigidBodyStatic*>(
					g_level->agTrace(g_level->grid_rigid_s, GetVehicle(),
					GetVehicle()->GetPos(), object->GetPos() - GetVehicle()->GetPos()) );

				TargetDesc td;
				td.target = object;
				td.bIsVisible = (NULL == pObstacle || pObstacle == object);

				targets.push_back(td);
			}
		}
	}

	for( size_t i = 0; i < targets.size(); ++i )
	{
		float l;
		if( targets[i].bIsVisible )
			l = (targets[i].target->GetPos() - GetVehicle()->GetPos()).len() / CELL_SIZE;
		else
			l = CreatePath( targets[i].target->GetPos().x,
			                targets[i].target->GetPos().y, AI_MAX_DEPTH, true, ws );

        if( l >= 0 )
		{
			AIPRIORITY p = GetTargetRate(targets[i].target) - AIP_NORMAL * l / AI_MAX_DEPTH;

			if( p > optimal )
			{
				optimal = p;
				pOptTarget = targets[i].target;
			}
		}
	}

	info.object   = pOptTarget;
	info.priority = optimal;

	return optimal > AIP_NOTREQUIRED;
}

bool GC_PlayerAI::FindItem(/*out*/ AIITEMINFO &info, const AIWEAPSETTINGS *ws)
{
	std::vector<GC_Pickup *> applicants;

	std::vector<OBJECT_LIST*> receive;
	g_level->grid_pickup.OverlapCircle(receive,
		GetVehicle()->GetPos().x / LOCATION_SIZE,
		GetVehicle()->GetPos().y / LOCATION_SIZE,
		AI_MAX_SIGHT * CELL_SIZE / LOCATION_SIZE );
	for( size_t i = 0; i < receive.size(); ++i )
	{
		OBJECT_LIST::iterator it = receive[i]->begin();
		for(; it != receive[i]->end(); ++it )
		{
			GC_Pickup *pItem = (GC_Pickup *) *it;
			if( pItem->IsAttached() || !pItem->IsVisible() || pItem->IsKilled() ) continue;

			if( (GetVehicle()->GetPos() - pItem->GetPos()).sqr() <
				(AI_MAX_SIGHT * CELL_SIZE) * (AI_MAX_SIGHT * CELL_SIZE) )
			{
				applicants.push_back(pItem);
			}
		}
	}


	AIPRIORITY optimal  = AIP_NOTREQUIRED;
	GC_Pickup *pOptItem = NULL;

	if( !applicants.empty() )
	{
		GC_Pickup *items[2] = {
			GetRawPtr(_pickupCurrent),
			applicants[g_level->net_rand() % applicants.size()]
		};
		for( int i = 0; i < 2; ++i )
		{
			if( NULL == items[i] ) continue;
			_ASSERT(!items[i]->IsKilled());
			_ASSERT(items[i]->IsVisible());
			if( items[i]->IsAttached() ) continue;
			float l = CreatePath(items[i]->GetPos().x, items[i]->GetPos().y, AI_MAX_DEPTH, true, ws);
			if( l >= 0 )
			{
				AIPRIORITY p = items[i]->GetPriority(GetVehicle()) - AIP_NORMAL * l / AI_MAX_DEPTH;
				if( items[i]->GetType() == _favoriteWeaponType )
				{
					if( GetVehicle()->GetWeapon()
						&& _favoriteWeaponType != GetVehicle()->GetWeapon()->GetType()
						&& !GetVehicle()->GetWeapon()->GetAdvanced() )
					{
						p += AIP_WEAPON_FAVORITE;
					}
				}
				if( p > optimal )
				{
					pOptItem = items[i];
					optimal = p;
				}
			}
		}
	}

	info.object   = pOptItem;
	info.priority = optimal;

	return optimal > AIP_NOTREQUIRED;
}

void GC_PlayerAI::SelectFavoriteWeapon()
{
	int wcount = 0;
	for( int i = 0; i < Level::GetTypeCount(); ++i )
	{
		ObjectType type = Level::GetTypeByIndex(i);
		const char *name = Level::GetTypeName(type);
		if( name == strstr(name, "weap_") )
		{
			if( 0 == g_level->net_rand() % ++wcount )
			{
				_favoriteWeaponType = type;
			}
		}
	}
}

// вычисление координат мнимой цели для стрельбы на опережение
void GC_PlayerAI::CalcOutstrip(GC_Vehicle *target, float Vp, vec2d &fake)
{
	ASSERT_TYPE(target, GC_Vehicle);

	float c = cosf(target->_angle);
	float s = sinf(target->_angle);

	float x = (target->GetPos().x - GetVehicle()->GetPos().x) * c +
	          (target->GetPos().y - GetVehicle()->GetPos().y) * s;
	float y = (target->GetPos().y - GetVehicle()->GetPos().y) * c -
	          (target->GetPos().x - GetVehicle()->GetPos().x) * s;

	float Vt = target->_lv.len();

	if( Vt < Vp )
	{
		float fx = x + Vt * (x * Vt + sqrtf(Vp*Vp * (y*y + x*x) - Vt*Vt * y*y)) / (Vp*Vp - Vt*Vt);

		fake.x = GetVehicle()->GetPos().x + fx * c - y * s;
		fake.y = GetVehicle()->GetPos().y + fx * s + y * c;
		fake.x = __max(0, __min(g_level->_sx, fake.x));
		fake.y = __max(0, __min(g_level->_sy, fake.y));
	}
	else
	{
		fake = GetVehicle()->GetPos();
	}
}

void GC_PlayerAI::SetL1(aiState_l1 new_state)
{
#ifdef _DEBUG
	if( _aiState_l1 == new_state )
		return;

	switch (new_state)
	{
	case L1_PATH_END:
//		REPORT("AI switch to L1_PATH_END\n");
		break;
	case L1_STICK:
//		REPORT("AI switch to L1_STICK\n");
		break;
	case L1_NONE:
//		REPORT("AI switch to L1_NONE\n");
		break;
	default:
//		REPORT("AI switch to L1<unknown>\n");
		_ASSERT(FALSE);
	}
#endif

	_aiState_l1 = new_state;
}

void GC_PlayerAI::SetL2(aiState_l2 new_state)
{
#ifdef _DEBUG
	if( _aiState_l2 == new_state )
		return;

	switch (new_state)
	{
	case L2_PATH_SELECT:
//		REPORT("AI switch to L2_PATH_SELECT\n");
		break;
	case L2_PICKUP:
//		REPORT("AI switch to L2_PICKUP\n");
		break;
	case L2_ATTACK:
		_ASSERT(_target);
//		REPORT("AI switch to L2_ATTACK\n");
		break;
	default:
//		REPORT("AI switch to L2<unknown>\n");
		_ASSERT(0);
	}
#endif

	_aiState_l2 = new_state;
}

// выбор действия
void GC_PlayerAI::ProcessAction(const AIWEAPSETTINGS *ws)
{
	AIITEMINFO ii_item;
	FindItem(ii_item, ws);

	AIITEMINFO ii_target;
	if( FindTarget(ii_target, ws) )
	{
		LockTarget((GC_RigidBodyStatic *) ii_target.object);

		if( ii_target.priority > ii_item.priority )
		{
			if( CreatePath(_target->GetPos().x, _target->GetPos().y, AI_MAX_DEPTH, false, ws) > 0 )
			{
				SmoothPath();
			}
			_pickupCurrent = NULL;
			SetL2(L2_ATTACK);
			SetL1(L1_NONE);
		}
		else
		{
			_ASSERT(ii_item.object);
			if( _pickupCurrent != ii_item.object )
			{
				if( CreatePath(ii_item.object->GetPos().x, ii_item.object->GetPos().y,
				               AI_MAX_DEPTH, false, ws) > 0 )
				{
					SmoothPath();
				}
				_pickupCurrent = (GC_Pickup *) ii_item.object;
			}
			SetL2(L2_PICKUP);
			SetL1(L1_NONE);
		}
	}
	else
	{
		FreeTarget();

		if( ii_item.priority > AIP_NOTREQUIRED )
		{
			_ASSERT(ii_item.object);
			if( _pickupCurrent != ii_item.object )
			{
				if( CreatePath(ii_item.object->GetPos().x, ii_item.object->GetPos().y,
				               AI_MAX_DEPTH, false, ws) > 0 )
				{
					SmoothPath();
				}
				_pickupCurrent = (GC_Pickup *) ii_item.object;
			}
			SetL2(L2_PICKUP);
			SetL1(L1_NONE);
		}
		else
		{
			_pickupCurrent = NULL;
			SetL2(L2_PATH_SELECT);
		}
	}
}

//Оценка ситуации, принятие решения
void GC_PlayerAI::SelectState(const AIWEAPSETTINGS *ws)
{
	_ASSERT(GetVehicle());

	GC_Pickup  *pItem    = NULL;
	GC_Vehicle *veh = NULL;

	ProcessAction(ws);

	switch( _aiState_l2 )
	{
	case L2_PICKUP: // едем прокачиваться
	{
	} break;
	case L2_ATTACK: // атакуем игрока
	{
		_ASSERT(_target);
	} break;
	case L2_PATH_SELECT:
	{
		_ASSERT(NULL == _target);
		if( L1_STICK == _aiState_l1 || _path.empty() )
		{
			vec2d t = GetVehicle()->GetPos()
				+ g_level->net_vrand(sqrtf(g_level->net_frand(1.0f))) * (AI_MAX_SIGHT * CELL_SIZE);
			float x = __min(__max(0, t.x), g_level->_sx);
			float y = __min(__max(0, t.y), g_level->_sy);

			if( CreatePath(x, y, AI_MAX_DEPTH, false, ws) > 0 )
			{
				SmoothPath();
				SetL1(L1_NONE);
			}
			else
				ClearPath();
		}
	} break;
	}
}

// Исполнение принятого решения
void GC_PlayerAI::DoState(VehicleState *pVehState, const AIWEAPSETTINGS *ws)
{
	if( L1_NONE != _aiState_l1 )
		return; // ожидание реакции на сообщение

	//
	// удаление ненужных узловых точек
	//

	bool bNeedStickCheck = true;

	vec2d destPoint = GetVehicle()->GetPos();

	if( !_path.empty() )
	{
		vec2d brake = GetVehicle()->GetBrakingLength();
		vec2d pos = GetVehicle()->GetPos() + brake;
		std::list<PathNode>::const_iterator nodeIt = FindNearPathNode(pos);
		if( (nodeIt->coord - pos).sqr() > CELL_SIZE*CELL_SIZE/4 )
		{
			destPoint = nodeIt->coord;
			if( ++nodeIt != _path.end() )
				destPoint = nodeIt->coord;
		}
		else
		{
			destPoint = pos + brake;
		}
	}

	static const TextureCache tex1("particle_1");
	(new GC_Particle(destPoint, vec2d(0,0), tex1, 0.5f))->SetFade(true);

/*
	while( !_path.empty() )
	{
		float desired = _path.size()>1 ? (_pickupCurrent ?
			_pickupCurrent->GetRadius() : 30.0f) : (float) CELL_SIZE / 2;
		float current = (GetVehicle()->GetPos() - _path.front().coord).len();
		if( current > desired )
		{
			break;
		}
		bNeedStickCheck = false;
		_path.pop_front();
	}

	if( !_path.empty() )
		destPoint = _path.front().coord;
*/

	bNeedStickCheck = false;

	// проверка убитости основной цели
	if( _target && _target->IsKilled() )
	{
		FreeTarget(); // free killed target
		ClearPath();
	}

	if( !GetVehicle()->GetWeapon() )
	{
		// no targets if no weapon
		FreeTarget();
		_attackList.clear();
	}


	//
	// пробуем атаковать основную цель
	//
	if( _target && IsTargetVisible(GetRawPtr(_target)))
	{
		_ASSERT(GetVehicle()->GetWeapon());

		vec2d fake = _target->GetPos();
		GC_Vehicle *enemy = dynamic_cast<GC_Vehicle *>(GetRawPtr(_target));
		if( ws->bNeedOutstrip && _level > 1 && enemy )
		{
			CalcOutstrip(enemy, ws->fProjectileSpeed, fake);
		}

		float len = (_target->GetPos() - GetVehicle()->GetPos()).len();
		if( len < ws->fAttackRadius_min )
		{
			RotateTo(pVehState, _target->GetPos(), false, true);
		}
		else
		{
			RotateTo(pVehState, _path.empty() ? _target->GetPos() : _path.front().coord,
				len > ws->fAttackRadius_max, false);
		}

		TowerTo(pVehState, fake, len > ws->fAttackRadius_crit, ws);
	}

	//
	// пробуем атаковать побочную цель
	//
	else if( !_attackList.empty() && IsTargetVisible(GetRawPtr(_attackList.front())) )
	{
		_ASSERT(GetVehicle()->GetWeapon());
		GC_RigidBodyStatic *target = GetRawPtr(_attackList.front());

		float len = (target->GetPos() - GetVehicle()->GetPos()).len();
		TowerTo(pVehState, target->GetPos(), len > ws->fAttackRadius_crit, ws);
		RotateTo(pVehState, _path.empty() ? target->GetPos() : destPoint,
			len > ws->fAttackRadius_max,
			len < ws->fAttackRadius_min);
	}

	//
	// обычное следование пути
	//
	else
	{
		pVehState->_bExplicitTower = false;
		pVehState->_bState_TowerCenter = true;
		if( !_path.empty() )
			RotateTo(pVehState, destPoint, true, false);
	}


	if( 0 && bNeedStickCheck )  // проверка на застревание
	if( GetVehicle()->_lv.len() < GetVehicle()->GetMaxSpeed() * 0.1f
		/* && engine_working_time > 1 sec */ )
	{
		// застряли :(
		SetL1(L1_STICK);
		_pickupCurrent = NULL;
	}
}

bool GC_PlayerAI::IsTargetVisible(GC_RigidBodyStatic *target, GC_RigidBodyStatic** ppObstacle)
{
	_ASSERT(GetVehicle()->GetWeapon());

	if( GC_Weap_Gauss::GetTypeStatic() == GetVehicle()->GetWeapon()->GetType() )  // FIXME!
		return true;

	GC_RigidBodyStatic *object = (GC_RigidBodyStatic *) g_level->agTrace(
		g_level->grid_rigid_s,
		GetVehicle(),
		GetVehicle()->GetPos(),
		target->GetPos() - GetVehicle()->GetPos() );

	if( object && object != target )
	{
		// может возникнуть ошибка, если цель убита
		if( ppObstacle ) *ppObstacle = object;
		return false;
	}
	else
	{
		if( ppObstacle ) *ppObstacle = NULL;
		return true;
	}
}

void GC_PlayerAI::OnRespawn()
{
	_jobManager.RegisterMember(this);
	SelectFavoriteWeapon();
}

void GC_PlayerAI::OnDie()
{
	_pickupCurrent = NULL;
	ClearPath();
	FreeTarget();

	_jobManager.UnregisterMember(this);
}

void GC_PlayerAI::LockTarget(GC_RigidBodyStatic *target)
{
	_ASSERT(target);
	_ASSERT(GetVehicle());
	_ASSERT(GetVehicle()->GetWeapon());

	if( target != _target )
	{
		FreeTarget();
		_target = target;
	}
}

void GC_PlayerAI::FreeTarget()
{
	_target = NULL;
}

////////////////////////////////////////////
// debug graphics

void GC_PlayerAI::debug_draw(HDC hdc)
{
	HPEN pen = CreatePen(PS_SOLID, 1, 0x00ffffff);
	HPEN oldpen = (HPEN) SelectObject(hdc, pen);

	if( !_path.empty() )
	{
		Ellipse(hdc, int(_path.front().coord.x) - 2, int(_path.front().coord.y) - 2,
					 int(_path.front().coord.x) + 2, int(_path.front().coord.y) + 2);

		std::list<PathNode>::iterator it = _path.begin();
		for(;;)
		{
			MoveToEx(hdc, int(it->coord.x), int(it->coord.y), NULL);
			if( ++it == _path.end() ) break;
			LineTo(hdc, int(it->coord.x), int(it->coord.y));
		}
	}

	if( _target )
	{
		SelectObject(hdc, oldpen);
		DeleteObject(pen);

		pen = CreatePen(PS_SOLID, 2, 0x00ff00ff);
		oldpen = (HPEN) SelectObject(hdc, pen);

		if( GetVehicle() )
		{
			MoveToEx(hdc, int(_target->GetPos().x), int(_target->GetPos().y), NULL);
			LineTo(hdc, int(GetVehicle()->GetPos().x), int(GetVehicle()->GetPos().y));
		}
	}

	SelectObject(hdc, oldpen);
	DeleteObject(pen);

	pen = CreatePen(PS_DOT, 1, 0x0000ff55);
	oldpen = (HPEN) SelectObject(hdc, pen);
	LOGBRUSH lb = {BS_HOLLOW};
	HBRUSH brush = CreateBrushIndirect(&lb);
	HBRUSH oldbrush = (HBRUSH) SelectObject(hdc, brush);

	SetBkMode(hdc, TRANSPARENT);

	/*
	CAttackList al(_AttackList);
	int count = 1;
	while( !al.IsEmpty() )
	{
		GC_RigidBodyStatic *target = al.Pop();
		FRECT frect;
		target->GetGlobalRect(frect);

		RECT rect;
		rect.left = (long) frect.left;
		rect.top = (long) frect.top;
		rect.right = (long) frect.right;
		rect.bottom = (long) frect.bottom;


		Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
		char s[20];
		wsprintf(s, "%d", count);
		DrawText(hdc, s, -1, &rect, DT_SINGLELINE|DT_CENTER|DT_VCENTER|DT_NOCLIP);
		++count;
	}
*/
	SelectObject(hdc, oldbrush);
	SelectObject(hdc, oldpen);
	DeleteObject(brush);
	DeleteObject(pen);
}

PropertySet* GC_PlayerAI::NewPropertySet()
{
	return new MyPropertySet(this);
}

GC_PlayerAI::MyPropertySet::MyPropertySet(GC_Object *object)
: BASE(object)
, _propLevel( ObjectProperty::TYPE_INTEGER, "level" )
{
	_propLevel.SetIntRange(0, AI_MAX_LEVEL);
}

int GC_PlayerAI::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 1;
}

ObjectProperty* GC_PlayerAI::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
	case 0: return &_propLevel;
	}

	_ASSERT(FALSE);
	return NULL;
}

void GC_PlayerAI::MyPropertySet::Exchange(bool applyToObject)
{
	BASE::Exchange(applyToObject);

	GC_PlayerAI *tmp = static_cast<GC_PlayerAI *>(GetObject());

	if( applyToObject )
	{
		tmp->SetLevel( _propLevel.GetIntValue() );
	}
	else
	{
		_propLevel.SetIntValue(tmp->_level);
	}
}


///////////////////////////////////////////////////////////////////////////////
// end of file
