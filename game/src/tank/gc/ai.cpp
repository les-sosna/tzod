// ai.cpp: implementation of the СAIController class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "core/JobManager.h"
#include "core/Debug.h"

#include "Options.h"
#include "ai.h"
#include "Macros.h"
#include "Functions.h"

#include "Vehicle.h"
#include "Turrets.h"
#include "Pickup.h"
#include "Player.h"


#include <d3dx9math.h>  // FIXME!



//////////////////////////////////////////////////////////////////////

#define NODE_RADIUS		32.0f
#define MIN_PATH_ANGLE	 0.4f

//////////////////////////////////////////////////////////////////////
// CAttackList class implementation

MemoryManager<CAttackList::tagAttackNode> CAttackList::s_anAllocator;

CAttackList::CAttackList()
{
	_firstTarget = NULL;
	_lastTarget  = NULL;
}

CAttackList::CAttackList(CAttackList &al)
{
	_firstTarget = NULL;
	_lastTarget  = NULL;
	*this = al;
}

CAttackList::~CAttackList()
{
	ClearList();
	_ASSERT(_firstTarget == NULL);
	_ASSERT(_lastTarget  == NULL);
}

CAttackList::tagAttackNode* CAttackList::FindObject(GC_RigidBodyStatic *object)
{
	tagAttackNode *pNode = _firstTarget;
	while (pNode)
	{
		if (object == pNode->_target) return pNode;
		pNode = pNode->_nextNode;
	}

	return NULL;
}

void CAttackList::RemoveFromList(tagAttackNode *pNode)
{
	_ASSERT(pNode);

	if (pNode == _firstTarget)
		_firstTarget = pNode->_nextNode;

	if (pNode == _lastTarget)
		_lastTarget = pNode->_prevNode;

	if (pNode->_prevNode)
		pNode->_prevNode->_nextNode = pNode->_nextNode;
	if (pNode->_nextNode)
		pNode->_nextNode->_prevNode = pNode->_prevNode;

	pNode->_target->Release();
	s_anAllocator.free(pNode);
}

GC_RigidBodyStatic* CAttackList::Pop(BOOL bRemoveFromList)
{
	_ASSERT(!IsEmpty());

	GC_RigidBodyStatic *target = _firstTarget->_target;
	if (bRemoveFromList) RemoveFromList(_firstTarget);

	return target;
}

void CAttackList::PushToBegin(GC_RigidBodyStatic *target)
{
	tagAttackNode *pNode = FindObject(target);
	if (pNode) RemoveFromList(pNode);

	tagAttackNode *pNewNode = s_anAllocator.allocate();
	pNewNode->_target = target;
	pNewNode->_target->AddRef();
	pNewNode->_prevNode = NULL;
	pNewNode->_nextNode = _firstTarget;
	if (_firstTarget) _firstTarget->_prevNode = pNewNode;
	_firstTarget = pNewNode;
	if (!_lastTarget) _lastTarget = pNewNode;
}

void CAttackList::PushToEnd(GC_RigidBodyStatic *target)
{
	tagAttackNode *pNode = FindObject(target);
	if (pNode) RemoveFromList(pNode);

	tagAttackNode *pNewNode = s_anAllocator.allocate();
	target->AddRef();
	pNewNode->_target = target;
	pNewNode->_nextNode = NULL;
	pNewNode->_prevNode = _lastTarget;
	if (_lastTarget) _lastTarget->_nextNode = pNewNode;
	_lastTarget = pNewNode;
	if (!_firstTarget) _firstTarget = pNewNode;
}

void CAttackList::Clean()
{
	tagAttackNode *pNode = _firstTarget;
	while (pNode)
	{
		if (pNode->_target->IsKilled())
		{
			tagAttackNode *pRem = pNode;
			pNode = pNode->_nextNode;
			RemoveFromList(pRem);
			continue;
		}

		pNode = pNode->_nextNode;
	}
}

CAttackList& CAttackList::operator= (CAttackList &al)
{
	ClearList();

	tagAttackNode *pNode = al._firstTarget;
	while (pNode)
	{
		PushToEnd(pNode->_target);
		pNode = pNode->_nextNode;
	}

	return al;
}


//////////////////////////////////////////////////////////////////////


#define GRID_ALIGN(x, sz)	((x)-(x)/(sz)*(sz)<(sz)/2)?((x)/(sz)):((x)/(sz)+1)

//std::list<AIController*> AIController::_controllers;
//std::list<AIController*>::iterator AIController::_itCurrent;

JobManager<AIController> AIController::_jobManager;

AIController::AIController(GC_Player *pPlayer) : CController(pPlayer)
{
	_target = NULL;

	if( !pPlayer->dead() )
	{
		_jobManager.RegisterMember(this);
	}

	Reset();
}

AIController::~AIController()
{
    Reset();
	if( !_player->dead() )
	{
		_jobManager.UnregisterMember(this);
	}
}

bool AIController::CheckCell(const FieldCell &cell)
{
	if( (0xFF != cell.Properties() && _player->_vehicle->_weapon) ||
		(0 == cell.Properties() && !_player->_vehicle->_weapon) )
	{
		return true;
	}
    return false;
}

float AIController::CreatePath(float dst_x, float dst_y, float max_depth, bool bTest)
{
	if( dst_x < 0 || dst_x >= g_level->_sx ||
		dst_y < 0 || dst_y >= g_level->_sy )
	{
		return -1;
	}

	Field::NewSession();
	Field &field = g_level->_field;

	std::priority_queue<
		RefFieldCell, 
		std::vector<RefFieldCell>, 
		std::greater<RefFieldCell> 
	> open;

	std::priority_queue<int> qqq;
	qqq.push(10);
	qqq.push(10);
	qqq.push(10);
	qqq.push(12);
	qqq.push(10);
	qqq.push(10);



	int start_x = GRID_ALIGN(int(_player->_vehicle->_pos.x), CELL_SIZE);
	int start_y = GRID_ALIGN(int(_player->_vehicle->_pos.y), CELL_SIZE);
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
			break;	// путь найден


		/* порядок проверки соседних узлов
		    4 | 0 | 6
		   ---+---+---
			2 | n | 3
		   ---+---+---
		    7 | 1 | 5
		*/					//   0  1  2  3  4  5  6  7
		static int   per_x[8] = {  0, 0,-1, 1,-1, 1, 1,-1 };	// смещение клетки по x
		static int   per_y[8] = { -1, 1, 0, 0,-1, 1,-1, 1 };	// смещение клетки по y
		static float dist [8] = {
			1.0f, 1.0f, 1.0f, 1.0f,
			1.4142f, 1.4142f, 1.4142f, 1.4142f };				// стоимость пути

		// для проверки проходимости по диагонали
		//                           4     5     6     7
		static int check_diag[] = { 0,2,  1,3,  3,0,  2,1 };

		for (int i = 0; i < 8; ++i)
		{
			if (i > 3) // проверка проходимости по диагонали
			if( !CheckCell(field(cn.GetX() + per_x[check_diag[(i-4)*2  ]], cn.GetY() + per_y[check_diag[(i-4)*2  ]])) ||
				!CheckCell(field(cn.GetX() + per_x[check_diag[(i-4)*2+1]], cn.GetY() + per_y[check_diag[(i-4)*2+1]]))   )
			{
				continue;
			}


			FieldCell &next = field(cn.GetX() + per_x[i], cn.GetY() + per_y[i]);
			if( CheckCell(next) )
			{
				// увеличение стоимости пути при пробивании сквозь стены
				float dist_mult = 1;
				if( 1 == next.Properties() )
					dist_mult = _weapSettings.fDistanceMultipler;

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
					_ASSERT(_player->_vehicle->_weapon);
					_ASSERT(cell->Properties() > 0);

					GC_Object *object = cell->GetObject(i);

					//
					// этот блок запрещает мочить свои стационарки.
					//  (не самая лучшая реализация)
					//
					if( _player->_team && OPT(nAIAccuracy) > 0 )
					{
						GC_Turret *pIsTurret = dynamic_cast<GC_Turret*>(object);
						if( pIsTurret && (pIsTurret->_team == _player->_team) )
						{
							continue;
						}
					}

					_AttackList.PushToBegin((GC_RigidBodyStatic *) object);
				}

				cell = cell->_prevCell;
			}
		}

		return distance;
	}

	// путь не найден
	return -1;
}

void AIController::SmoothPath()
{
	if( _path.size() < 4 ) return;

	int init_size = _path.size();

	D3DXVECTOR2 vn[4];
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
		vn[i].x = it[i]->coord.x;
		vn[i].y = it[i]->coord.y;
		_ASSERT(vn[i].x > 0 && vn[i].y > 0);
	}

	while( true )
	{
		D3DXVECTOR2 insert;
		PathNode new_node;

		for( int i = 1; i < 4; ++i )
		{
			D3DXVec2CatmullRom(&insert, &vn[0], &vn[1], &vn[2], &vn[3], (float) i / 4.0f);
			_ASSERT(insert.x > 0 && insert.y > 0);

			new_node.coord.x = insert.x;
			new_node.coord.y = insert.y;

			_path.insert(it[2], new_node);
		}

		for( int i = 0; i < 3; ++i )
		{
			it[i] = it[i+1];
			vn[i] = vn[i+1];
		}

		if( ++it[3] == _path.end() )
			break;

		vn[3].x = it[3]->coord.x;
		vn[3].y = it[3]->coord.y;
	}
}

void AIController::ClearPath()
{
	_path.clear();
	_AttackList.ClearList();
}

void AIController::RotateTo(VehicleState *pState, const vec2d &x, bool bForv, bool bBack)
{
	_ASSERT(!_isnan(x.x) && !_isnan(x.y));
	_ASSERT(_finite(x.x) && _finite(x.y));
	_ASSERT(x.x > 0 && x.y > 0);

	float ang2 = (x - _player->_vehicle->_pos).Angle();
	float ang1 = _player->_vehicle->_angle;

	float d1 = fabsf(ang2-ang1);
	float d2 = ang1 < ang2 ? ang1-ang2+PI2 : ang2-ang1+PI2;

	if( (d1 < MIN_PATH_ANGLE || d2 < MIN_PATH_ANGLE) && bForv )
		pState->_bState_MoveForvard	= true;

	if( (d1 < MIN_PATH_ANGLE || d2 < MIN_PATH_ANGLE) && bBack )
		pState->_bState_MoveBack	= true;

	pState->_bExplicitBody = true;
	pState->_fBodyAngle = ang2;
}

void AIController::TowerTo(VehicleState *pState, const vec2d &x, bool bFire)
{
	_ASSERT(_player->_vehicle);
	_ASSERT(_player->_vehicle->_weapon);

	float ang2 = (x - _player->_vehicle->_pos).Angle() + _current_offset;
	float ang1 = _player->_vehicle->_angle + _player->_vehicle->_weapon->_angle;
	if (ang1 > PI2) ang1 -= PI2;


	float d1 = fabsf(ang2-ang1);
	float d2 = ang1 < ang2 ? ang1-ang2+PI2 : ang2-ang1+PI2;

	if( (d1 < _weapSettings.fMaxAttackAngle || 
		 d2 < _weapSettings.fMaxAttackAngle) && bFire)
	{
		pState->_bState_Fire = true;
	}

	pState->_bExplicitTower = true;
	pState->_fTowerAngle = ang2 - _player->_vehicle->_angle; //_player->_vehicle->_rotator.geta();
	//--------------------------------
	_ASSERT(!_isnan(pState->_fTowerAngle) && _finite(pState->_fTowerAngle));
}

// оценка полезности атаки данной цели
AIPRIORITY AIController::GetTargetRate(GC_Vehicle *target)
{
	_ASSERT(target);
	_ASSERT(_player->_vehicle);
	_ASSERT(_player->_vehicle->_weapon);

	if (0 != target->_player->_team &&
		target->_player->_team == _player->_team)
	{
		return AIP_NOTREQUIRED;	// своих не атакуем
	}

	AIPRIORITY p = AIP_NORMAL;

	p += AIP_NORMAL * (_player->_vehicle->GetHealth() / _player->_vehicle->GetHealthMax());
	p -= AIP_NORMAL * (target->GetHealth() / target->GetHealthMax());

	return p;
}

// return TRUE еслт цель найдена
bool AIController::FindTarget(/*out*/ AIITEMINFO &info)
{
	if (!_player->_vehicle->_weapon) return FALSE;

	AIPRIORITY optimal = AIP_NOTREQUIRED;
	GC_Vehicle *pOptTarget = NULL;

	std::vector<TargetDesc> targets;

	//
	// проверяем цели
	//

	ENUM_BEGIN(vehicles, GC_Vehicle, object)
	{
		if (!object->IsKilled() && object != _player->_vehicle)
		{
			if( (_player->_vehicle->_pos - object->_pos).Square() <
				(AI_MAX_SIGHT * CELL_SIZE) * (AI_MAX_SIGHT * CELL_SIZE) )
			{
				GC_RigidBodyStatic *pObstacle = (GC_RigidBodyStatic *)
					g_level->agTrace(
						g_level->grid_rigid_s, GetRawPtr(_player->_vehicle),
						_player->_vehicle->_pos, object->_pos - _player->_vehicle->_pos);

				TargetDesc td;
				td.target = object;
				td.bIsVisible = (NULL == pObstacle || pObstacle == object);

				targets.push_back(td);
			}
		}
	} ENUM_END();

	for( size_t i = 0; i < targets.size(); ++i )
	{
		float l;
		if( targets[i].bIsVisible )
			l = (targets[i].target->_pos - _player->_vehicle->_pos).Length() / CELL_SIZE;
		else
			l = CreatePath(targets[i].target->_pos.x, targets[i].target->_pos.y, AI_MAX_DEPTH, true);

        if( l >= 0 )
		{
			AIPRIORITY p = GetTargetRate(targets[i].target) - AIP_NORMAL * l / AI_MAX_DEPTH;

			if (p > optimal)
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

bool AIController::FindItem(/*out*/ AIITEMINFO &info)
{
	std::vector<GC_PickUp *> applicants;

	std::vector<OBJECT_LIST*> receive;
	g_level->grid_pickup.OverlapCircle(receive,
		_player->_vehicle->_pos.x / LOCATION_SIZE,
		_player->_vehicle->_pos.y / LOCATION_SIZE,
		AI_MAX_SIGHT * CELL_SIZE / LOCATION_SIZE );
	for( size_t i = 0; i < receive.size(); ++i )
	{
		OBJECT_LIST::iterator it = receive[i]->begin();
		for(; it != receive[i]->end(); ++it )
		{
			GC_PickUp *pItem = (GC_PickUp *) *it;
			if (pItem->_bAttached || !pItem->IsVisible() || pItem->IsKilled()) continue;

			if( (_player->_vehicle->_pos - pItem->_pos).Square() <
				(AI_MAX_SIGHT * CELL_SIZE) * (AI_MAX_SIGHT * CELL_SIZE) )
			{
				applicants.push_back(pItem);
			}
		}
	}


	AIPRIORITY optimal  = AIP_NOTREQUIRED;
	GC_PickUp *pOptItem = NULL;

	if( !applicants.empty() )
	{
		GC_PickUp *items[2] = { GetRawPtr(_pickupCurrent), applicants[net_rand() % applicants.size()] };
		for( int i = 0; i < 2; ++i )
		{
			if( NULL == items[i] ) continue;
			_ASSERT(!items[i]->IsKilled());
			_ASSERT(items[i]->IsVisible());
			if( items[i]->_bAttached ) continue;
			float l = CreatePath(items[i]->_pos.x, items[i]->_pos.y, AI_MAX_DEPTH, true);
			if( l >= 0 )
			{
				AIPRIORITY p = items[i]->CheckUseful(GetRawPtr(_player->_vehicle)) - AIP_NORMAL * l / AI_MAX_DEPTH;
				if( items[i]->GetType() == _otFavoriteWeapon )
				{
					if( _player->_vehicle->_weapon )
					if( _otFavoriteWeapon != _player->_vehicle->_weapon->GetType() &&
						!_player->_vehicle->_weapon->IsAdvanced() )
					{
						p += AIP_WEAPON_FAVORITE;
					}
				}
				if (p > optimal)
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

// вычисление координат мнимой цели для стрельбы на опережение
void AIController::CalcOutstrip(GC_Vehicle *target, float Vp, vec2d &fake)
{
	ASSERT_TYPE(target, GC_Vehicle);

	float gamma = target->_angle;
	float Vt = target->_lv.Length();

	float cg = cosf(gamma), sg = sinf(gamma);

	float x = (target->_pos.x - _player->_vehicle->_pos.x) * cg +
			  (target->_pos.y - _player->_vehicle->_pos.y) * sg;
	float y = (target->_pos.y - _player->_vehicle->_pos.y) * cg -
			  (target->_pos.x - _player->_vehicle->_pos.x) * sg;

	float fx = x + Vt * (x * Vt + sqrtf(Vp*Vp * (y*y + x*x) - Vt*Vt * y*y)) / (Vp*Vp - Vt*Vt);

	if( _isnan(fx) || !_finite(fx) )
	{
		fake = _player->_vehicle->_pos;
	}
	else
	{
		fake.x = _player->_vehicle->_pos.x + fx*cg - y*sg;
		fake.y = _player->_vehicle->_pos.y + fx*sg + y*cg;

		fake.x = __max(0, __min(g_level->_sx, fake.x));
		fake.y = __max(0, __min(g_level->_sy, fake.y));
	}
}

void AIController::SetL1(AIController::aiState_l1 new_state)
{
#ifdef _DEBUG
	if( _aiState_l1 == new_state )
		return;

	switch (new_state)
	{
	case L1_PATH_END:
		REPORT("AI switch to L1_PATH_END\n");
		break;
	case L1_STICK:
		REPORT("AI switch to L1_STICK\n");
		break;
	case L1_NONE:
		REPORT("AI switch to L1_NONE\n");
		break;
	default:
		REPORT("AI switch to L1<unknown>\n");
		_ASSERT(0);
	}
#endif

	_aiState_l1 = new_state;
}

void AIController::SetL2(AIController::aiState_l2 new_state)
{
#ifdef _DEBUG
	if( _aiState_l2 == new_state )
		return;

	switch (new_state)
	{
	case L2_PATH_SELECT:
		REPORT("AI switch to L2_PATH_SELECT\n");
		break;
	case L2_PICKUP:
		REPORT("AI switch to L2_PICKUP\n");
		break;
	case L2_ATTACK:
		_ASSERT(_target);
		REPORT("AI switch to L2_ATTACK\n");
		break;
	default:
		REPORT("AI switch to L2<unknown>\n");
		_ASSERT(0);
	}
#endif

	_aiState_l2 = new_state;
}

// выбор действия
void AIController::ProcessAction()
{
	AIITEMINFO ii_item;
	FindItem(ii_item);

	AIITEMINFO ii_target;
	if( FindTarget(ii_target) )
	{
		LockTarget((GC_RigidBodyStatic *) ii_target.object);

		if( ii_target.priority > ii_item.priority )
		{
			if( CreatePath(_target->_pos.x, _target->_pos.y, AI_MAX_DEPTH, false) > 0 )
				SmoothPath();
			_pickupCurrent = NULL;
			SetL2(L2_ATTACK);
			SetL1(L1_NONE);
		}
		else
		{
			_ASSERT(ii_item.object);
			if( _pickupCurrent != ii_item.object )
			{
				if( CreatePath(ii_item.object->_pos.x, ii_item.object->_pos.y, AI_MAX_DEPTH, false) > 0 )
					SmoothPath();
				_pickupCurrent = (GC_PickUp *) ii_item.object;
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
				if( CreatePath(ii_item.object->_pos.x, ii_item.object->_pos.y, AI_MAX_DEPTH, false) > 0 )
					SmoothPath();
				_pickupCurrent = (GC_PickUp *) ii_item.object;
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
void AIController::SelectState()
{
	_ASSERT(_player->_vehicle);

	GC_PickUp  *pItem    = NULL;
	GC_Vehicle *pVehicle = NULL;

	ProcessAction();

	switch (_aiState_l2)
	{
	case L2_PICKUP:	// едем прокачиваться
	{
	} break;
	case L2_ATTACK:	// атакуем игрока
	{
		_ASSERT(_target);
	} break;
	case L2_PATH_SELECT:
	{
		_ASSERT(NULL == _target);
		if( L1_STICK == _aiState_l1 || _path.empty() )
		{
			vec2d t = _player->_vehicle->_pos + net_vrand(sqrtf(net_frand(1.0f))) * (AI_MAX_SIGHT * CELL_SIZE);
			float x = __min(__max(0, t.x), g_level->_sx);
			float y = __min(__max(0, t.y), g_level->_sy);

			if( CreatePath(x, y, AI_MAX_DEPTH, false) > 0 )
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
void AIController::DoState(VehicleState *pVehState)
{
	if( L1_NONE != _aiState_l1 )
		return;	// ожидание реакции на сообщение

	//
	// удаление ненужных узловых точек
	//

	bool bNeedStickCheck = true;
	while( !_path.empty() )
	{
		float desired = _path.size()>1 ? (_pickupCurrent ?
			_pickupCurrent->getRadius() : 50.0f) : (float) CELL_SIZE / 2;
		float current = (_player->_vehicle->_pos - _path.front().coord).Length();
		if ( current > desired )
		{
			break;
		}
		bNeedStickCheck = false;
		_path.pop_front();
	}

	// проверка убитости основной цели
	if( NULL != _target && _target->IsKilled() )
	{
		FreeTarget();	// освобождаем убитую цель
		ClearPath();
	}

	//
	// пробуем атаковать основную цель
	//
	if( _target && IsTargetVisible(_target))
	{
		_ASSERT(_player->_vehicle->_weapon);

		vec2d fake = _target->_pos;			//координаты мнимой цели
		if( _weapSettings.bNeedOutstrip && OPT(nAIAccuracy) > 1 &&
			dynamic_cast<GC_Vehicle *>(_target) )
		{
			CalcOutstrip((GC_Vehicle *)_target, _weapSettings.fProjectileSpeed, fake);
		}

		float len = (_target->_pos - _player->_vehicle->_pos).Length();
		if( len < _weapSettings.fAttackRadius_min )
			RotateTo(pVehState, _target->_pos, false, true);
		else
			RotateTo(pVehState, _path.empty() ? _target->_pos : _path.front().coord,
				len > _weapSettings.fAttackRadius_max, false);

		TowerTo(pVehState, fake, len > _weapSettings.fAttackRadius_crit);
	}

	//
	// пробуем атаковать побочную цель
	//
	else if( !_AttackList.IsEmpty() && IsTargetVisible(_AttackList.Pop(FALSE)) )
	{
		_ASSERT(_player->_vehicle->_weapon);
		GC_RigidBodyStatic *target = _AttackList.Pop(FALSE);

		float len = (target->_pos - _player->_vehicle->_pos).Length();
		TowerTo(pVehState, target->_pos,
			len > _weapSettings.fAttackRadius_crit);
		RotateTo(pVehState, _path.empty() ? target->_pos : _path.front().coord,
			len > _weapSettings.fAttackRadius_max,
			len < _weapSettings.fAttackRadius_min);
	}

	//
	// обычное следование пути
	//
	else
	{
		pVehState->_bExplicitTower = false;
		pVehState->_bState_TowerCenter = true;
		if( !_path.empty() )
			RotateTo(pVehState, _path.front().coord, true, false);
	}


	if( 0 && bNeedStickCheck )  // проверка на застревание
	if (_player->_vehicle->_lv.Length() < _player->_vehicle->_MaxForvSpeed * 0.1
		/* && engine_working_time > 1 sec */)
	{
		// застряли :(
		SetL1(L1_STICK);
		_pickupCurrent = NULL;
	}

}

bool AIController::IsTargetVisible(GC_RigidBodyStatic *target, GC_RigidBodyStatic** ppObstacle)
{
	_ASSERT(_player->_vehicle->_weapon);

	if( GC_Weap_Gauss::this_type == _player->_vehicle->_weapon->GetType() )
		return true;

	GC_RigidBodyStatic *object = (GC_RigidBodyStatic *) g_level->agTrace(
		g_level->grid_rigid_s,
		GetRawPtr(_player->_vehicle),
		_player->_vehicle->_pos,
		target->_pos - _player->_vehicle->_pos);

	if( object && object != target )
	{
		// может возникнуть ошибка, если цель убита
		if (ppObstacle) *ppObstacle = object;
		return false;
	}
	else
	{
		if (ppObstacle) *ppObstacle = NULL;
		return true;
	}
}

void AIController::Reset()
{
	REPORT("AI controller reset\n");

	_pickupCurrent = NULL;

	ClearPath();
	FreeTarget();

/*
	static const ObjectType weaponzzz[] = {
		OT_WEAP_AUTOCANNON, OT_WEAP_ROCKETLAUNCHER,
		OT_WEAP_CANON, OT_WEAP_GAUSS, OT_WEAP_RAM,
		OT_WEAP_BFG, OT_WEAP_RIPPER, OT_WEAP_MINIGUN };

	_otFavoriteWeapon = weaponzzz[net_rand() % (sizeof(weaponzzz) / sizeof(enumObjectType))];
*/

	_desired_offset = 0;
	_current_offset = 0;

	SetL2(L2_PATH_SELECT);
	SetL1(L1_NONE);
}

void AIController::OnPlayerRespawn()
{
	_jobManager.RegisterMember(this);
}

void AIController::OnPlayerDie()
{
	_jobManager.UnregisterMember(this);
}

void AIController::GetControl(VehicleState *pState, float dt)
{
	_ASSERT(!_player->dead());
	ZeroMemory(pState, sizeof(VehicleState));
	_AttackList.Clean();

	if( !_player->dead() && _pickupCurrent )
	{
		if( _pickupCurrent->IsKilled() || !_pickupCurrent->IsVisible() )
		{
			_pickupCurrent = NULL;
		}
		else if( (_pickupCurrent->_pos - _player->_vehicle->_pos).Square() <
					_pickupCurrent->getRadius() * _pickupCurrent->getRadius() )
		{
			pState->_bState_AllowDrop = true;
		}
	}

	// настройка оружия
	if( _player->_vehicle->_weapon )
		_player->_vehicle->_weapon->SetupAI(&_weapSettings);


	// принятие решения
	if( _jobManager.TakeJob(this) )
		SelectState();


	// установка _current_offset для понижения меткости стрельбы
	const float acc_speed = 0.5f;	// скорость движения мнимой цели
	if( dynamic_cast<GC_Vehicle *>(_target) )
	{
		float len = fabsf(_desired_offset - _current_offset);
		if( acc_speed*dt >= len )
		{
			_current_offset = _desired_offset;

			static float d_array[5] = {0.176f, 0.122f, 0.09f, 0.05f, 0.00f};

			float d = d_array[OPT(nAIAccuracy)];

			if( OPT(nAIAccuracy) > 2 )
			{
				d = d_array[OPT(nAIAccuracy)] *
					fabsf(((GC_Vehicle *)_target)->_lv.Length()) /
					((GC_Vehicle *)_target)->_MaxForvSpeed;
			}

			if( d > 0 )
				_desired_offset = net_frand(d);
			else
				_desired_offset = 0;
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
	DoState(pState);


	//
	// управление фарами
	//
	switch(OPT(nAIAccuracy))
	{
	case 0:
	case 1:
		pState->_bLight = true;
		break;
	case 2:
	case 3:
		pState->_bLight = (NULL != _target);
		break;
	default:
		pState->_bLight = false;
	}
}

void AIController::LockTarget(GC_RigidBodyStatic *target)
{
	_ASSERT(target);
	_ASSERT(_player->_vehicle);
	_ASSERT(_player->_vehicle->_weapon);

	if( target != _target )
	{
		FreeTarget();

		_target = target;
		_target->AddRef();

		REPORT("AI locks a new target\n");
	}
}

void AIController::FreeTarget()
{
	if( _target )
	{
		_target->Release();
		_target = NULL;

		REPORT("AI free target\n");
	}
}

////////////////////////////////////////////
// debug graphics
/*
#ifdef _DRAW_GDI
void AIController::debug_draw(HDC hdc)
{
	HPEN pen = CreatePen(PS_SOLID, 1, 0x00ffffff);
	HPEN oldpen = (HPEN) SelectObject(hdc, pen);

	if( !_path.empty() )
	{
		Ellipse(hdc, int(_path.front().coord.x) - 2, int(_path.front().coord.y) - 2,
					 int(_path.front().coord.x) + 2, int(_path.front().coord.y) + 2);

		std::list<PathNode>::iterator it = _path.begin();
		while(true)
		{
			MoveToEx(hdc, int(it->coord.x), int(it->coord.y), NULL);
			if( ++it == _path.end() ) break;
			LineTo(hdc, int(it->coord.x), int(it->coord.y));
		}
	}

	if (_target)
	{
		SelectObject(hdc, oldpen);
		DeleteObject(pen);

		pen = CreatePen(PS_SOLID, 2, 0x00ff00ff);
		oldpen = (HPEN) SelectObject(hdc, pen);

		if (_player->_vehicle)
		{
			MoveToEx(hdc, int(_target->_pos.x), int(_target->_pos.y), NULL);
			LineTo(hdc, int(_player->_vehicle->_pos.x), int(_player->_vehicle->_pos.y));
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

	CAttackList al(_AttackList);
	int count = 1;
	while (!al.IsEmpty())
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

	SelectObject(hdc, oldbrush);
	SelectObject(hdc, oldpen);
	DeleteObject(brush);
	DeleteObject(pen);
}
#endif
*/

///////////////////////////////////////////////////////////////////////////////
// end of file
