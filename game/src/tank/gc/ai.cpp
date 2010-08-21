// ai.cpp: implementation of the GC_PlayerAI class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "ai.h"

#include "core/JobManager.h"
#include "core/Debug.h"
#include "config/Config.h"

#include "fs/SaveFile.h"
#include "fs/MapFile.h"

#include "Macros.h"
#include "Functions.h"

#include "Vehicle.h"
#include "Turrets.h"
#include "Pickup.h"
#include "Player.h"
#include "Weapons.h"
#include "crate.h"

#include "Camera.h"

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
#define MIN_PATH_ANGLE       0.8f

#define GRID_ALIGN(x, sz)    ((x)-(x)/(sz)*(sz)<(sz)/2)?((x)/(sz)):((x)/(sz)+1)

///////////////////////////////////////////////////////////////////////////////

JobManager<GC_PlayerAI> GC_PlayerAI::_jobManager;

IMPLEMENT_SELF_REGISTRATION(GC_PlayerAI)
{
	ED_SERVICE("ai", "obj_service_player_ai");
	return true;
}

GC_PlayerAI::GC_PlayerAI()
  : _isActive(true)
  , _desiredOffset(0)
  , _currentOffset(0)
  , _arrivalPoint(0,0)
  , _level(2)
  , _backTime(0)
  , _stickTime(0)
  , _favoriteWeaponType(INVALID_OBJECT_TYPE)
{
	SetL2(L2_PATH_SELECT);
	SetL1(L1_NONE);
}

GC_PlayerAI::GC_PlayerAI(FromFile)
  : GC_Player(FromFile())
{
}

GC_PlayerAI::~GC_PlayerAI()
{
}

void GC_PlayerAI::Serialize(SaveFile &f)
{
	GC_Player::Serialize(f);

	f.Serialize(_level);
	f.Serialize(_aiState_l1);
	f.Serialize(_aiState_l2);
	f.Serialize(_currentOffset);
	f.Serialize(_desiredOffset);
	f.Serialize(_favoriteWeaponType);
	f.Serialize(_pickupCurrent);
	f.Serialize(_target);
	f.Serialize(_arrivalPoint);
	f.Serialize(_backTime);
	f.Serialize(_stickTime);
	f.Serialize(_isActive);

	if( f.loading() )
	{
		size_t size;
		f.Serialize(size);
		while( size-- )
		{
			_attackList.push_back(AttackListType::value_type());
			f.Serialize(_attackList.back());
		}

		if( GetVehicle() )
		{
			_jobManager.RegisterMember(this);
		}
	}
	else
	{
		size_t size = _attackList.size();
		f.Serialize(size);
		for( AttackListType::iterator it = _attackList.begin(); _attackList.end() != it; ++it )
		{
			f.Serialize(*it);
		}
	}
}

void GC_PlayerAI::MapExchange(MapFile &f)
{
	GC_Player::MapExchange(f);
	MAP_EXCHANGE_INT(level, _level, 0);
	MAP_EXCHANGE_INT(active, _isActive, true);
}

void GC_PlayerAI::TimeStepFixed(float dt)
{
	GC_Player::TimeStepFixed(dt);

	if( !GetVehicle() )
	{
		return;
	}


//	GC_Camera::GetWorldMousePos(_arrivalPoint);


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
		if( _pickupCurrent->IsKilled() || !_pickupCurrent->GetVisible() )
		{
			_pickupCurrent = NULL;
		}
		else if( (_pickupCurrent->GetPos() - GetVehicle()->GetPos()).sqr() <
		          _pickupCurrent->GetRadius() * _pickupCurrent->GetRadius() )
		{
			vs._bState_AllowDrop = true;
		}
	}

	AIWEAPSETTINGS weapSettings;
	if( GetVehicle()->GetWeapon() )
		GetVehicle()->GetWeapon()->SetupAI(&weapSettings);

//	return;

	// take decision
	if( _jobManager.TakeJob(this) )
		SelectState(&weapSettings);


	// select a _currentOffset to reduce shooting accuracy
	const float acc_speed = 0.4f; // angular velocity of a fake target
	if( dynamic_cast<GC_Vehicle *>(GetRawPtr(_target)) )
	{
		float len = fabsf(_desiredOffset - _currentOffset);
		if( acc_speed*dt >= len )
		{
			_currentOffset = _desiredOffset;

			static float d_array[5] = {0.186f, 0.132f, 0.09f, 0.05f, 0.00f};

			float d = d_array[_level];

			if( _level > 2 )
			{
				d = d_array[_level] *
					fabsf(static_cast<GC_Vehicle*>(GetRawPtr(_target))->_lv.len()) /
						static_cast<GC_Vehicle*>(GetRawPtr(_target))->GetMaxSpeed();
			}

			_desiredOffset = (d > 0) ? (g_level->net_frand(d) - d * 0.5f) : 0;
		}
		else
		{
			_currentOffset += (_desiredOffset - _currentOffset) * dt * acc_speed / len;
		}
	}
	else
	{
		_desiredOffset = 0;
		_currentOffset = 0;
	}

	// realize the decision
	DoState(&vs, &weapSettings);
	
	_backTime -= dt;

	if( (vs._bState_MoveForward || vs._bState_MoveBack) &&
	    GetVehicle()->_lv.len() < GetVehicle()->GetMaxSpeed() * 0.1f )
	{
		_stickTime += dt;
		if( _stickTime > 0.6f )
		{
			_backTime = g_level->net_frand(0.5f);
		}
	}
	else
	{
		_stickTime = 0;
	}


	//
	// headlight control
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
	GetVehicle()->TimeStepFixed(dt);
	if( GetVehicle() ) // vehicle might be killed in its time step
	{
		GetVehicle()->GetVisual()->Sync(GetVehicle()); // FIXME: cat tracks	
		if (g_conf.sv_nightmode.Get()) vs._bLight = true;
		else vs._bLight = false;
		GetVehicle()->SetPredictedState(vs);
		GetVehicle()->GetVisual()->TimeStepFixed(dt);
	}
}

bool GC_PlayerAI::CheckCell(const FieldCell &cell) const
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
	start.UpdatePath(0, end_x, end_y);
	start._prevCell  = NULL;

	open.push( RefFieldCell(start) );


	while( !open.empty() )
	{
		FieldCell &cn = open.top();
		open.pop();

		if( cn.GetX() == end_x && cn.GetY() == end_y )
			break; // a path was found


		// neighbor nodes check order
		//    4 | 0 | 6
		//   ---+---+---
		//    2 | n | 3
		//   ---+---+---
		//    7 | 1 | 5
		//                         0  1  2  3  4  5  6  7
		static int   per_x[8] = {  0, 0,-1, 1,-1, 1, 1,-1 };  // node x offset
		static int   per_y[8] = { -1, 1, 0, 0,-1, 1,-1, 1 };  // node y offset
		static float dist [8] = {
			1.0f, 1.0f, 1.0f, 1.0f,
			1.4142f, 1.4142f, 1.4142f, 1.4142f };             // path cost

		// for diagonal checks
		//                           4     5     6     7
		static int check_diag[] = { 0,2,  1,3,  3,0,  2,1 };

		for( int i = 0; i < 8; ++i )
		{
			if( i > 3 ) // check diagonal passability
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
				// increase path cost when travel through the walls
				float dist_mult = 1;
				if( 1 == next.Properties() )
					dist_mult = ws->fDistanceMultipler;

				if( !next.IsChecked() )
				{
					next._prevCell  = &cn;
					next.UpdatePath(cn.Before() + dist[i] * dist_mult, end_x, end_y);
					next.Check();
					//-----------------
					if( next.Total() < max_depth )
						open.push(RefFieldCell(next));
				}

				// next part of code causes assertions in <algorithm> because
				// it can modify cells that are being stored in "open" queue

				//else if( next._before > cn._before + dist[i] * dist_mult )
				//{
				//	next._before = cn._before + dist[i] * dist_mult;
				//	next._prevCell  = &cn;
				//	//-----------------
				//	if( next.Total() < max_depth )
				//		open.push(RefFieldCell(next));
				//}
			}
		}
	}

	if( field(end_x, end_y).IsChecked() )
	{
		// a path was found
		const FieldCell *cell = &field(end_x, end_y);
		float distance = cell->Before();

		if( !bTest )
		{
			//
			// construct a new path
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
					assert(GetVehicle()->GetWeapon());
					assert(cell->Properties() > 0);

					GC_RigidBodyStatic *object = cell->GetObject(i);

					//
					// this piece of code protects friendly turrets.
					//  (could be implemented better)
					//
					if( GetTeam() && _level > 0 )
					{
						GC_Turret *pIsTurret = dynamic_cast<GC_Turret*>(object);
						if( pIsTurret && (pIsTurret->_team == GetTeam()) )
						{
							continue;
						}
					}
					_attackList.push_front(WrapRawPtr(object));
				}

				cell = cell->_prevCell;
			}
		}

		return distance;
	}

	// path not found
	return -1;
}

void GC_PlayerAI::SmoothPath()
{
	if( _path.size() < 4 ) 
		return;

	int init_size = _path.size();

	vec2d vn[4];
	std::list<PathNode>::iterator it[4], tmp;

	//
	// smooth angles
	//
	if( _path.size() > 4 )
	{
		it[1] = _path.begin();
		it[0] = it[1]++;
		while( it[1] != _path.end() )
		{
			PathNode new_node;
			new_node.coord = (it[0]->coord + it[1]->coord) * 0.5f;
			assert(new_node.coord.x > 0 && new_node.coord.y > 0);
			_path.insert(it[1], new_node);
			if( it[0] != _path.begin() )
				_path.erase(it[0]);
			it[0] = it[1]++;
		}
	}


	//
	// spline interpolation
	//

	tmp = _path.begin();
	for( int i = 0; i < 4; ++i )
	{
		it[i] = tmp++;
		vn[i] = it[i]->coord;
		assert(vn[i].x > 0 && vn[i].y > 0);
	}

	for(;;)
	{
		PathNode new_node;

		for( int i = 1; i < 4; ++i )
		{
			CatmullRom(vn[0], vn[1], vn[2], vn[3], new_node.coord, (float) i / 4.0f);
			assert(new_node.coord.x > 0 && new_node.coord.y > 0);
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

std::list<GC_PlayerAI::PathNode>::const_iterator GC_PlayerAI::FindNearPathNode(
	const vec2d &pos, vec2d *projection, float *offset) const
{
	assert(!_path.empty());
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

	assert(_path.end() != result);

	if( projection )
	{
		vec2d dev = pos - result->coord;

		float prevL = -1;
		float nextL = -1;

		vec2d prevPos;
		vec2d nextPos;

		vec2d nextDir;
		vec2d prevDir;

		float nextDir2;
		float nextDot;

		float prevDir2;
		float prevDot;

		std::list<PathNode>::const_iterator prevIt = result;
		if( _path.begin() != result )
		{
			--prevIt;
			prevDir  = prevIt->coord - result->coord;
			prevDir2 = prevDir.sqr();
			prevDot  = prevDir * dev;
			prevL    = prevDot / sqrtf(prevDir2);
		}

		std::list<PathNode>::const_iterator nextIt = result; ++nextIt;
		if( _path.end() != nextIt )
		{
			nextDir  = nextIt->coord - result->coord;
			nextDir2 = nextDir.sqr();
			nextDot  = nextDir * dev;
			nextL    = nextDot / sqrtf(nextDir2);
		}

		if( prevL > 0 && prevL > nextL )
		{
			vec2d d = prevDir * (prevDot / prevDir2);
			prevPos  = result->coord + d;
			*projection = prevPos;
//			result = prevIt;
			if( offset ) 
			{
				*offset = d.len();
			}
		}
		else
		if( nextL > 0 && nextL > prevL )
		{
			vec2d d = nextDir * (nextDot / nextDir2);
			nextPos  = result->coord + d;
			*projection = nextPos;
			if( offset )
			{
				*offset = -d.len();
			}
		}
		else
		{
			*projection = result->coord;
			if( offset )
			{
				*offset = 0;
			}
		}
	}

	return result;
}

void GC_PlayerAI::ClearPath()
{
	_path.clear();
	_attackList.clear();
}
void GC_PlayerAI::DoDefaultStats()
{

}

void GC_PlayerAI::RotateTo(VehicleState *pState, const vec2d &x, bool bForv, bool bBack)
{
	assert(!_isnan(x.x) && !_isnan(x.y));
	assert(_finite(x.x) && _finite(x.y));
	assert(GetVehicle());

	vec2d tmp = x - GetVehicle()->GetPos();
	tmp.Normalize();

	float cosDiff = tmp * GetVehicle()->GetDirection();
	float minDiff = std::cos(MIN_PATH_ANGLE);

	pState->_bState_MoveForward = cosDiff > minDiff && bForv;
	pState->_bState_MoveBack = cosDiff > minDiff && bBack;

	pState->_bExplicitBody = true;
	pState->_fBodyAngle = tmp.Angle();
}

void GC_PlayerAI::TowerTo(VehicleState *pState, const vec2d &x, bool bFire, const AIWEAPSETTINGS *ws)
{
	assert(GetVehicle());
	assert(GetVehicle()->GetWeapon());

	vec2d tmp = x - GetVehicle()->GetPos();
	if( tmp.x && tmp.y )
	{
		tmp.Normalize();
		tmp = Vec2dAddDirection(tmp, vec2d(_currentOffset));
		float cosDiff = tmp * GetVehicle()->GetWeapon()->GetDirectionReal();
		pState->_bState_Fire = bFire && cosDiff >= ws->fMaxAttackAngleCos;
		pState->_bExplicitTower = true;
		pState->_fTowerAngle = Vec2dSubDirection(tmp, GetVehicle()->GetDirection()).Angle() - GetVehicle()->GetSpinup();
		assert(!_isnan(pState->_fTowerAngle) && _finite(pState->_fTowerAngle));
	}
	else
	{
		pState->_bState_Fire = bFire;
		pState->_bExplicitTower = false;
		pState->_fTowerAngle = 0;
		pState->_bState_TowerLeft = false;
		pState->_bState_TowerRight = false;
		pState->_bState_TowerCenter = false;
	}
}

AIPRIORITY GC_PlayerAI::GetTargetRateBox(GC_Crate *targetBox)
{
	assert(targetBox);
	assert(GetVehicle());
	assert(GetVehicle()->GetWeapon());

	AIPRIORITY p = AIP_NORMAL;

	p += AIP_NORMAL * (GetVehicle()->GetHealth() / GetVehicle()->GetHealthMax());
	p -= AIP_NORMAL * (targetBox->GetHealth() / targetBox->GetHealthMax());

	return p;
}


// evaluate the rate of attacking of the given target
AIPRIORITY GC_PlayerAI::GetTargetRate(GC_Vehicle *target)
{
	assert(target);
	assert(GetVehicle());
	assert(GetVehicle()->GetWeapon());

	if( !target->GetOwner() ||
		(0 != target->GetOwner()->GetTeam() && target->GetOwner()->GetTeam() == GetTeam()) )
	{
		return AIP_NOTREQUIRED; // don't attack friends and playerless vehicles
	}

	AIPRIORITY p = AIP_NORMAL;

	p += AIP_NORMAL * (GetVehicle()->GetHealth() / GetVehicle()->GetHealthMax());
	p -= AIP_NORMAL * (target->GetHealth() / target->GetHealthMax());

	return p;
}

// return TRUE if a target was found
bool GC_PlayerAI::FindTarget(/*out*/ AIITEMINFO &info, const AIWEAPSETTINGS *ws)
{
	if( !GetVehicle()->GetWeapon() ) return FALSE;

	AIPRIORITY optimal = AIP_NOTREQUIRED;
	GC_Vehicle *pOptTarget = NULL;
	GC_RigidBodyStatic *pOptTargetBox = NULL;

	std::vector<TargetDesc> targets;

	//
	// check targets
	//

	FOREACH( g_level->GetList(LIST_vehicles), GC_Vehicle, object ) //для каждого транспортного средства
	{
		if( !object->GetOwner() ||
			(0 != object->GetOwner()->GetTeam() && object->GetOwner()->GetTeam() == GetTeam()) )
		{
			continue;
		}

		if( !object->IsKilled() && object != GetVehicle() ) //если чел не убит и это не мы
		{
			if( (GetVehicle()->GetPos() - object->GetPos()).sqr() <
				(AI_MAX_SIGHT * CELL_SIZE) * (AI_MAX_SIGHT * CELL_SIZE) ) //если в поле зрения
			{
				GC_RigidBodyStatic *pObstacle = static_cast<GC_RigidBodyStatic*>(
					g_level->TraceNearest(g_level->grid_rigid_s, GetVehicle(),
					GetVehicle()->GetPos(), object->GetPos() - GetVehicle()->GetPos()) );

				TargetDesc td;
				td.target = object;
				td.bIsVisible = (NULL == pObstacle || pObstacle == object);
				td.bIsObject=0;
				//заполняем общюю таблицу для дальнейшей обработки
				targets.push_back(td);
			}
		}
	}


		FOREACH( g_level->GetList(LIST_objects), GC_Object, object) //для каждого динамического объекта
	{
		if (GC_Crate *it = dynamic_cast<GC_Crate *>(object) )
		{
		if( !object->IsKilled()) //если чел не убит и это не мы
		{
			if( (GetVehicle()->GetPos() - it->GetPos()).sqr() <
				(AI_MAX_SIGHT * CELL_SIZE) * (AI_MAX_SIGHT * CELL_SIZE) ) //если в поле зрения
			{
				GC_RigidBodyStatic *pObstacle = static_cast<GC_RigidBodyStatic*>(
					g_level->TraceNearest(g_level->grid_rigid_s, GetVehicle(),
					GetVehicle()->GetPos(), it->GetPos() - GetVehicle()->GetPos()) );

				TargetDesc td;
				td.targetBox = it;
				td.bIsObject=1;
				td.bIsVisible = (NULL == pObstacle || pObstacle == it);
				targets.push_back(td);
			}
		}
		}
	}



	for( size_t i = 0; i < targets.size(); ++i ) //находим самого близкого / ближний дин. объект
	{
		float l;
		if (targets[i].bIsObject==1){
				if( targets[i].bIsVisible )
			l = (targets[i].targetBox->GetPos() - GetVehicle()->GetPos()).len() / CELL_SIZE;
		else
			l = CreatePath( targets[i].targetBox->GetPos().x,
			                targets[i].targetBox->GetPos().y, AI_MAX_DEPTH-40, true, ws );
		
		}
		else
		{
		if( targets[i].bIsVisible )
			l = (targets[i].target->GetPos() - GetVehicle()->GetPos()).len() / CELL_SIZE;
		else
			l = CreatePath( targets[i].target->GetPos().x,
			                targets[i].target->GetPos().y, AI_MAX_DEPTH, true, ws );
		}
        if( l >= 0 )
		{
			AIPRIORITY p;
			if (targets[i].bIsObject==1)
			p= GetTargetRateBox(targets[i].targetBox) - AIP_NORMAL * l / AI_MAX_DEPTH;
			else
			p = GetTargetRate(targets[i].target) - AIP_NORMAL * l / AI_MAX_DEPTH;

			if( p > optimal )
			{
				optimal = p;
				if (targets[i].bIsObject==1)
				pOptTargetBox = targets[i].targetBox;
				else pOptTarget = targets[i].target;
			}
		}
	}
	if (pOptTarget)
	info.object   = WrapRawPtr(pOptTarget);
	else info.object   = WrapRawPtr(pOptTargetBox);
	info.priority = optimal;

	return optimal > AIP_NOTREQUIRED;
}

bool GC_PlayerAI::FindItem(/*out*/ AIITEMINFO &info, const AIWEAPSETTINGS *ws)
{
	std::vector<GC_Pickup *> applicants;

	PtrList<ObjectList> receive;
	FRECT rt = {
		(GetVehicle()->GetPos().x - AI_MAX_SIGHT * CELL_SIZE) / LOCATION_SIZE,
		(GetVehicle()->GetPos().y - AI_MAX_SIGHT * CELL_SIZE) / LOCATION_SIZE,
		(GetVehicle()->GetPos().x + AI_MAX_SIGHT * CELL_SIZE) / LOCATION_SIZE,
		(GetVehicle()->GetPos().y + AI_MAX_SIGHT * CELL_SIZE) / LOCATION_SIZE};

	g_level->grid_pickup.OverlapRect(receive, rt);
	for( PtrList<ObjectList>::iterator i = receive.begin(); i != receive.end(); ++i )
	{
		ObjectList::iterator it = (*i)->begin();
		for(; it != (*i)->end(); ++it )
		{
			GC_Pickup *pItem = (GC_Pickup *) *it;
			if( pItem->GetCarrier() || !pItem->GetVisible() || pItem->IsKilled() ) 
			{
				continue;
			}

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
			assert(!items[i]->IsKilled());
			assert(items[i]->GetVisible());
			if( items[i]->GetCarrier() ) continue;
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

	info.object   = WrapRawPtr(pOptItem);
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

// calculates the position of a fake target for more accurate shooting
void GC_PlayerAI::CalcOutstrip(GC_Vehicle *target, float Vp, vec2d &fake)
{
	ASSERT_TYPE(target, GC_Vehicle);
	float Vt = target->_lv.len();
	if( Vt < Vp )
	{
		float c = target->GetDirection().x;
		float s = target->GetDirection().y;

		float x = (target->GetPos().x - GetVehicle()->GetPos().x) * c +
		          (target->GetPos().y - GetVehicle()->GetPos().y) * s;
		float y = (target->GetPos().y - GetVehicle()->GetPos().y) * c -
		          (target->GetPos().x - GetVehicle()->GetPos().x) * s;

		float fx = x + Vt * (x * Vt + sqrt(Vp*Vp * (y*y + x*x) - Vt*Vt * y*y)) / (Vp*Vp - Vt*Vt);

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
		assert(false);
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
		assert(_target);
//		REPORT("AI switch to L2_ATTACK\n");
		break;
	default:
//		REPORT("AI switch to L2<unknown>\n");
		assert(0);
	}
#endif

	_aiState_l2 = new_state;
}

void GC_PlayerAI::ProcessAction(const AIWEAPSETTINGS *ws)
{
	AIITEMINFO ii_item;
	FindItem(ii_item, ws);

	AIITEMINFO ii_target;
	if( FindTarget(ii_target, ws) )
	{
		LockTarget(SafePtrCast<GC_RigidBodyStatic>(ii_target.object));

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
			Pickup(GetRawPtr(SafePtrCast<GC_Pickup>(ii_item.object)));
		}
	}
	else
	{
		FreeTarget();

		if( ii_item.priority > AIP_NOTREQUIRED )
		{
			assert(ii_item.object);
			if( _pickupCurrent != ii_item.object )
			{
				if( CreatePath(ii_item.object->GetPos().x, ii_item.object->GetPos().y,
				               AI_MAX_DEPTH, false, ws) > 0 )
				{
					SmoothPath();
				}
				_pickupCurrent = SafePtrCast<GC_Pickup>(ii_item.object);
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

bool GC_PlayerAI::March(float x, float y)
{
	if( GetVehicle() )
	{
		AIWEAPSETTINGS ws;
		if( GetVehicle()->GetWeapon() )
			GetVehicle()->GetWeapon()->SetupAI(&ws);
		if( CreatePath(x, y, AI_MAX_DEPTH, false, &ws) > 0 )
		{
			SmoothPath();
			SetL1(L1_NONE);
			return true;
		}
	}
	return false;
}

bool GC_PlayerAI::Attack(GC_RigidBodyStatic *target)
{
	if( GetVehicle() && GetVehicle()->GetWeapon() )
	{
		if( target )
		{
			LockTarget(WrapRawPtr(target));
		}
		else
		{
			FreeTarget();
		}
		return true;
	}
	return false;
}


bool GC_PlayerAI::Pickup(GC_Pickup *p)
{
	assert(p);
	if( GetVehicle() )
	{
		if( _pickupCurrent != p )
		{
			AIWEAPSETTINGS ws;
			if( GetVehicle()->GetWeapon() )
				GetVehicle()->GetWeapon()->SetupAI(&ws);

			if( CreatePath(p->GetPos().x, p->GetPos().y, AI_MAX_DEPTH, false, &ws) > 0 )
			{
				SmoothPath();
				_pickupCurrent = WrapRawPtr(p);
				SetL2(L2_PICKUP);
				SetL1(L1_NONE);
				return true;
			}
		}
		else
		{
			return true;
		}
	}
	return false;
}

void GC_PlayerAI::Stop()
{
	FreeTarget();
	ClearPath();
}

void GC_PlayerAI::SelectState(const AIWEAPSETTINGS *ws)
{
	assert(GetVehicle());
	if( !_isActive )
	{
		return;
	}
	

	GC_Pickup  *pItem = NULL;
	GC_Vehicle *veh = NULL;

	ProcessAction(ws);
	
	switch( _aiState_l2 )
	{
	case L2_PICKUP:
	{
	} break;
	case L2_ATTACK:
	{
		assert(_target);
	} break;
	case L2_PATH_SELECT:
	{
		assert(NULL == _target);
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

void GC_PlayerAI::SetActive(bool active)
{
	_isActive = active;
}

void GC_PlayerAI::DoState(VehicleState *pVehState, const AIWEAPSETTINGS *ws)
{
	if( L1_NONE != _aiState_l1 )
		return;


	vec2d brake = GetVehicle()->GetBrakingLength();
	float brake_len = brake.len();
	float brakeSqr = brake.sqr();

	vec2d currentDir = GetVehicle()->GetDirection();
	vec2d currentPos = GetVehicle()->GetPos();
	vec2d predictedPos = GetVehicle()->GetPos() + brake;

	if( !_path.empty() )
	{
		vec2d predictedProj;
		std::list<PathNode>::const_iterator predictedNodeIt = FindNearPathNode(predictedPos, &predictedProj, NULL);

		vec2d currentProj;
		float offset;
//		GC_Camera::GetWorldMousePos(currentPos);
		std::list<PathNode>::const_iterator currentNodeIt = FindNearPathNode(currentPos, &currentProj, &offset);

		float desiredProjOffsetLen = GetVehicle()->GetMaxBrakingLength() * 2;//(1 + GetVehicle()->_lv.len() / GetVehicle()->GetMaxSpeed());
		vec2d desiredProjOffset;

		std::list<PathNode>::const_iterator it = currentNodeIt;
		if( offset > 0 )
		{
			vec2d d = it->coord;
			--it;
			d -= it->coord;
			offset -= d.len();
		}
		offset += __min((currentPos - currentProj).len(), desiredProjOffsetLen);
		for(;;)
		{
			vec2d d = it->coord;
			if( ++it == _path.end() )
			{
				desiredProjOffset = d;
				break;
			}
			d -= it->coord;
			float len = d.len();
			if( offset + len < desiredProjOffsetLen )
			{
				offset += len;
			}
			else
			{
				float ratio = 1 - (desiredProjOffsetLen - offset) / len;
				desiredProjOffset = it->coord + d * ratio;
				break;
			}
		}

		if( ++currentNodeIt == _path.end() && (currentProj - currentPos).sqr() < CELL_SIZE*CELL_SIZE/16 )
		{
			_path.clear(); // end of path
		}
		//else
		//{
		//	if( (predictedPos - predictedProj).sqr() < CELL_SIZE*CELL_SIZE && brake_len > 1 )
		//	{
		//		_arrivalPoint = predictedProj + brake;
		//	}
		//	else
		//	{
				_arrivalPoint = desiredProjOffset;
		//	}
		//}

	}


//	GC_Camera::GetWorldMousePos(_arrivalPoint);

	

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
		_path.pop_front();
	}

	if( !_path.empty() )
		destPoint = _path.front().coord;
*/

	pVehState->_bExplicitBody = false;
	pVehState->_bExplicitTower = false;
	pVehState->_bState_TowerCenter = true;



	// check if the primary target is still alive
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
	// attack the primary target if possible
	//
	if( _target && IsTargetVisible(GetRawPtr(_target)))
	{
		assert(GetVehicle()->GetWeapon());

		vec2d fake = _target->GetPos();
		GC_Vehicle *enemy = dynamic_cast<GC_Vehicle *>(GetRawPtr(_target));
		if( ws->bNeedOutstrip && _level > 1 && enemy )
		{
			CalcOutstrip(enemy, ws->fProjectileSpeed, fake);
		}

		float len = (_target->GetPos() - GetVehicle()->GetPos()).len();
		TowerTo(pVehState, fake, len > ws->fAttackRadius_crit, ws);

		vec2d d = _target->GetPos() - currentPos;
		float d_len = d.len();
		if( d_len < ws->fAttackRadius_min )
		{
			if( d_len < 1 )
			{
				d = -currentDir;
				d_len = 1;
			}
			_arrivalPoint = currentPos + d * (1 - ws->fAttackRadius_min / d_len);
		}
	}

	//
	// attack secondary targets of possible
	//
	else if( !_attackList.empty() && IsTargetVisible(GetRawPtr(_attackList.front())) )
	{
		assert(GetVehicle()->GetWeapon());
		GC_RigidBodyStatic *target = GetRawPtr(_attackList.front());

		float len = (target->GetPos() - GetVehicle()->GetPos()).len();
		TowerTo(pVehState, target->GetPos(), len > ws->fAttackRadius_crit, ws);
	}


	//
	// avoid obstacles
	//

	if( brake.sqr() > 0 )
	{
		vec2d angle[] = {vec2d(PI/4), vec2d(0), vec2d(-PI/4)};
		float len[] = {1,2,1};

		float min_d = -1;
		vec2d min_hit, min_norm;
		for( int i = 0; i < 3; ++i )
		{
			vec2d tmp = Vec2dAddDirection(GetVehicle()->GetDirection(), vec2d(angle[i]));

			vec2d x0 = GetVehicle()->GetPos() + tmp * GetVehicle()->GetRadius();
			vec2d a  = brake * len[i];

			vec2d hit, norm;
			GC_Object *o = g_level->TraceNearest(
				g_level->grid_rigid_s, 
				GetVehicle(), 
				x0,
				a,
				&hit,
				&norm
			);

			if( o )
			{
				if (GC_Crate *it = dynamic_cast<GC_Crate *>(o) )
				{
				} else {
				if( 1 == i && (hit - currentPos).len() < GetVehicle()->GetRadius() )
				{
					_backTime = 0.5f;
				}

				float d = (hit - x0).sqr();
				if( min_d < 0 || d < min_d )
				{
					min_d = d;
					min_hit = hit;
					min_norm = norm;
				}
				}
			}
		}

		if( min_d > 0 )
		{
			vec2d hit_dir = currentDir; // (min_hit - currentPos).Normalize();
			min_norm = (min_norm - hit_dir * (min_norm * hit_dir)).Normalize() + min_norm;
			min_norm.Normalize();
			min_norm *= 1.4142f;// sqrt(2)
			_arrivalPoint = min_hit + min_norm * GetVehicle()->GetRadius();
			g_level->DbgLine(min_hit, _arrivalPoint, 0xff0000ff);
		}
	}

	g_level->DbgLine(GetVehicle()->GetPos(), _arrivalPoint, 0x0000ffff);


	//
	// follow the path
	//

	float dst = (currentPos - _arrivalPoint).sqr();
	if( dst > CELL_SIZE*CELL_SIZE/16 )
	{
		if( _backTime <= 0 )
		{
			RotateTo(pVehState, _arrivalPoint, dst > brakeSqr, false);
		}
		else
		{
			pVehState->_bState_MoveBack = true;
		}
	}


	//if( GetVehicle()->_lv.len() < GetVehicle()->GetMaxSpeed() * 0.1f
	//	/* && engine_working_time > 1 sec */ )
	//{
	//	// got stuck :(
	//	SetL1(L1_STICK);
	//	_pickupCurrent = NULL;
	//}
}

bool GC_PlayerAI::IsTargetVisible(GC_RigidBodyStatic *target, GC_RigidBodyStatic** ppObstacle)
{
	assert(GetVehicle()->GetWeapon());

	if( GC_Weap_Gauss::GetTypeStatic() == GetVehicle()->GetWeapon()->GetType() )  // FIXME!
		return true;

	GC_RigidBodyStatic *object = (GC_RigidBodyStatic *) g_level->TraceNearest(
		g_level->grid_rigid_s,
		GetVehicle(),
		GetVehicle()->GetPos(),
		target->GetPos() - GetVehicle()->GetPos() );

	if( object && object != target )
	{
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
	_arrivalPoint = GetVehicle()->GetPos();
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

void GC_PlayerAI::LockTarget(const SafePtr<GC_RigidBodyStatic> &target)
{
	assert(target);
	assert(GetVehicle());
	assert(GetVehicle()->GetWeapon());

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

void GC_PlayerAI::debug_draw()
{
	if( !_path.empty() )
	{
//		Ellipse(hdc, int(_path.front().coord.x) - 2, int(_path.front().coord.y) - 2,
//					 int(_path.front().coord.x) + 2, int(_path.front().coord.y) + 2);

		std::list<PathNode>::iterator it = _path.begin();
		for(;;)
		{
			vec2d v = it->coord;
			if( ++it == _path.end() ) break;
			g_level->DbgLine(v, it->coord, 0xffffffff);
		}
	}
/*
	if( _target )
	{
		if( GetVehicle() )
		{
			g_render->DrawLine(_target->GetPos(), GetVehicle()->GetPos(), 0xff00ffff);
		}
	}
*/

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

}

PropertySet* GC_PlayerAI::NewPropertySet()
{
	return new MyPropertySet(this);
}

GC_PlayerAI::MyPropertySet::MyPropertySet(GC_Object *object)
  : BASE(object)
  , _propLevel( ObjectProperty::TYPE_INTEGER, "level" )
  , _propActive( ObjectProperty::TYPE_INTEGER, "active" )
{
	_propLevel.SetIntRange(0, AI_MAX_LEVEL);
	_propActive.SetIntRange(0, 1);
}

int GC_PlayerAI::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 2;
}

ObjectProperty* GC_PlayerAI::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
	default: assert(false);
	case 0: return &_propLevel;
	case 1: return &_propActive;
	}
}

void GC_PlayerAI::MyPropertySet::MyExchange(bool applyToObject)
{
	BASE::MyExchange(applyToObject);

	GC_PlayerAI *tmp = static_cast<GC_PlayerAI *>(GetObject());

	if( applyToObject )
	{
		tmp->SetLevel( _propLevel.GetIntValue() );
		tmp->SetActive( 0 != _propActive.GetIntValue() );
	}
	else
	{
		_propLevel.SetIntValue(tmp->_level);
		_propActive.SetIntValue(tmp->GetActive());
	}
}


///////////////////////////////////////////////////////////////////////////////
// end of file
