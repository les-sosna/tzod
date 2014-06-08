// ai.cpp

#include "ai.h"

#include "Macros.h"
#include "MapFile.h"
#include "SaveFile.h"

#include "gc/Camera.h"
#include "gc/Pickup.h"
#include "gc/Player.h"
#include "gc/Turrets.h"
#include "gc/Vehicle.h"
#include "gc/Weapons.h"
#include "gc/World.h"

#include "core/JobManager.h"
#include "core/Debug.h"

#include <queue>

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

//JobManager<GC_PlayerAI> GC_PlayerAI::_jobManager;


AIController::AIController()
  : _arrivalPoint(0,0)
  , _desiredOffset(0)
  , _currentOffset(0)
  , _favoriteWeaponType(INVALID_OBJECT_TYPE)
  , _level(2)
  , _backTime(0)
  , _stickTime(0)
  , _isActive(true)
{
	SetL2(L2_PATH_SELECT);
	SetL1(L1_NONE);
}

AIController::AIController(FromFile)
{
}

AIController::~AIController()
{
}

void AIController::Serialize(SaveFile &f)
{
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

//		if( GetVehicle() )
//		{
//			_jobManager.RegisterMember(this);
//		}
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

void AIController::ReadControllerState(World &world, float dt, const GC_Vehicle &vehicle, VehicleState &vs)
{
	if( vehicle.GetOwner() )
	{
		return;
	}


//	GC_Camera::GetWorldMousePos(_arrivalPoint);


	memset(&vs, 0, sizeof(VehicleState));

	// clean the attack list
	_attackList.remove_if( [](const ObjPtr<GC_RigidBodyStatic> &arg) -> bool { return !arg; } );

	if( _pickupCurrent )
	{
		if( !_pickupCurrent->GetVisible() )
		{
			_pickupCurrent = NULL;
		}
		else if( (_pickupCurrent->GetPos() - vehicle.GetPos()).sqr() <
		          _pickupCurrent->GetRadius() * _pickupCurrent->GetRadius() )
		{
			vs._bState_AllowDrop = true;
		}
	}

	AIWEAPSETTINGS weapSettings;
	if( vehicle.GetWeapon() )
		vehicle.GetWeapon()->SetupAI(&weapSettings);


	// take decision
//	if( _jobManager.TakeJob(this) )
        SelectState(world, vehicle, vehicle.GetWeapon() ? &weapSettings : nullptr);


	// select a _currentOffset to reduce shooting accuracy
	const float acc_speed = 0.4f; // angular velocity of a fake target
	if( PtrDynCast<GC_Vehicle>(_target) )
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
					fabs(PtrCast<GC_Vehicle>(_target)->_lv.len()) /
						PtrCast<GC_Vehicle>(_target)->GetMaxSpeed();
			}

			_desiredOffset = (d > 0) ? (world.net_frand(d) - d * 0.5f) : 0;
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
	DoState(world, vehicle, &vs, &weapSettings);
	
	_backTime -= dt;

	if( (vs._bState_MoveForward || vs._bState_MoveBack) && vehicle._lv.len() < vehicle.GetMaxSpeed() * 0.1f )
	{
		_stickTime += dt;
		if( _stickTime > 0.6f )
		{
			_backTime = world.net_frand(0.5f);
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
}

// check the cell's passability taking into account current weapon settings
static bool CheckCell(const FieldCell &cell, bool hasWeapon)
{
	return (0xFF != cell.Properties() && hasWeapon) || (0 == cell.Properties() && !hasWeapon);
}

float AIController::CreatePath(World &world, vec2d from, vec2d to, int team, float max_depth, bool bTest, const AIWEAPSETTINGS *ws)
{
	if( to.x < 0 || to.x >= world._sx || to.y < 0 || to.y >= world._sy )
	{
		return -1;
	}

	Field::NewSession();
	Field &field = world._field;

	std::priority_queue<
		RefFieldCell, std::vector<RefFieldCell>, std::greater<RefFieldCell>
	> open;

	int start_x = GRID_ALIGN(int(from.x), CELL_SIZE);
	int start_y = GRID_ALIGN(int(from.y), CELL_SIZE);
	int end_x = GRID_ALIGN(int(to.x), CELL_SIZE);
	int end_y = GRID_ALIGN(int(to.y), CELL_SIZE);

	FieldCell &start = field(start_x, start_y);

	if( !CheckCell(start, !!ws) ) return -1;

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
			                     cn.GetY() + per_y[check_diag[(i-4)*2  ]]), !!ws) ||
			    !CheckCell(field(cn.GetX() + per_x[check_diag[(i-4)*2+1]],
			                     cn.GetY() + per_y[check_diag[(i-4)*2+1]]), !!ws) )
			{
				continue;
			}


			FieldCell &next = field(cn.GetX() + per_x[i], cn.GetY() + per_y[i]);
			if( CheckCell(next, !!ws) )
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

			node.coord = to;
			_path.push_front(node);

			cell = cell->_prevCell;
			while( NULL != cell )
			{
				node.coord.x = (float) (cell->GetX() * CELL_SIZE);
				node.coord.y = (float) (cell->GetY() * CELL_SIZE);
				_path.push_front(node);

				for( int i = 0; i < cell->GetObjectsCount(); ++i )
				{
					assert(ws);
					assert(cell->Properties() > 0);

					GC_RigidBodyStatic *object = cell->GetObject(i);

					//
					// this piece of code protects friendly turrets.
					//  (could be implemented better)
					//
					if( team && _level > 0 )
					{
						GC_Turret *pIsTurret = dynamic_cast<GC_Turret*>(object);
						if( pIsTurret && (pIsTurret->_team == team) )
						{
							continue;
						}
					}
					_attackList.push_front(object);
				}

				cell = cell->_prevCell;
			}
		}

		return distance;
	}

	// path not found
	return -1;
}

void AIController::SmoothPath()
{
	if( _path.size() < 4 ) 
		return;

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

std::list<AIController::PathNode>::const_iterator AIController::FindNearPathNode(
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

void AIController::ClearPath()
{
	_path.clear();
	_attackList.clear();
}

static void RotateTo(const GC_Vehicle &vehicle, VehicleState *pState, const vec2d &x, bool bForv, bool bBack)
{
	assert(!isnan(x.x) && !isnan(x.y));
	assert(isfinite(x.x) && isfinite(x.y));

	vec2d tmp = x - vehicle.GetPos();
	tmp.Normalize();

	float cosDiff = tmp * vehicle.GetDirection();
	float minDiff = std::cos(MIN_PATH_ANGLE);

	pState->_bState_MoveForward = cosDiff > minDiff && bForv;
	pState->_bState_MoveBack = cosDiff > minDiff && bBack;

	pState->_bExplicitBody = true;
	pState->_fBodyAngle = tmp.Angle();
}

void AIController::TowerTo(const GC_Vehicle &vehicle, VehicleState *pState, const vec2d &x, bool bFire, const AIWEAPSETTINGS *ws)
{
	assert(vehicle.GetWeapon());

	vec2d tmp = x - vehicle.GetPos();
	if( tmp.x && tmp.y )
	{
		tmp.Normalize();
		tmp = Vec2dAddDirection(tmp, vec2d(_currentOffset));
		float cosDiff = tmp * vehicle.GetWeapon()->GetDirection();
		pState->_bState_Fire = bFire && cosDiff >= ws->fMaxAttackAngleCos;
		pState->_bExplicitTower = true;
		pState->_fTowerAngle = Vec2dSubDirection(tmp, vehicle.GetDirection()).Angle() - vehicle.GetSpinup();
		assert(!isnan(pState->_fTowerAngle) && isfinite(pState->_fTowerAngle));
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

// evaluate the rate of attacking of the given target
AIPRIORITY AIController::GetTargetRate(const GC_Vehicle &vehicle, GC_Vehicle &target)
{
	assert(vehicle.GetWeapon());

	if( !target.GetOwner() ||
		(0 != target.GetOwner()->GetTeam() && target.GetOwner()->GetTeam() == vehicle.GetOwner()->GetTeam()) )
	{
		return AIP_NOTREQUIRED; // don't attack friends and playerless vehicles
	}

	AIPRIORITY p = AIP_NORMAL;

	p += AIP_NORMAL * (vehicle.GetHealth() / vehicle.GetHealthMax());
	p -= AIP_NORMAL * (target.GetHealth() / target.GetHealthMax());

	return p;
}

// return true if a target was found
bool AIController::FindTarget(World &world, const GC_Vehicle &vehicle, /*out*/ AIITEMINFO &info, const AIWEAPSETTINGS *ws)
{
	if( !vehicle.GetWeapon() )
        return false;

	AIPRIORITY optimal = AIP_NOTREQUIRED;
	GC_Vehicle *pOptTarget = NULL;

	std::vector<TargetDesc> targets;

	//
	// check targets
	//

	FOREACH( world.GetList(LIST_vehicles), GC_Vehicle, object )
	{
		if( !object->GetOwner() ||
			(0 != object->GetOwner()->GetTeam() && object->GetOwner()->GetTeam() == vehicle.GetOwner()->GetTeam()) )
		{
			continue;
		}

		if( object != &vehicle )
		{
			if( (vehicle.GetPos() - object->GetPos()).sqr() <
				(AI_MAX_SIGHT * CELL_SIZE) * (AI_MAX_SIGHT * CELL_SIZE) )
			{
				GC_RigidBodyStatic *pObstacle = static_cast<GC_RigidBodyStatic*>(
					world.TraceNearest(world.grid_rigid_s, &vehicle,
					vehicle.GetPos(), object->GetPos() - vehicle.GetPos()) );

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
			l = (targets[i].target->GetPos() - vehicle.GetPos()).len() / CELL_SIZE;
		else
			l = CreatePath(world, vehicle.GetPos(), targets[i].target->GetPos(), vehicle.GetOwner()->GetTeam(), AI_MAX_DEPTH, true, ws);

        if( l >= 0 )
		{
            assert(targets[i].target);
			AIPRIORITY p = GetTargetRate(vehicle, *targets[i].target) - AIP_NORMAL * l / AI_MAX_DEPTH;

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

bool AIController::FindItem(World &world, const GC_Vehicle &vehicle, /*out*/ AIITEMINFO &info, const AIWEAPSETTINGS *ws)
{
	std::vector<GC_Pickup *> applicants;

    std::vector<ObjectList*> receive;
	FRECT rt = {
		(vehicle.GetPos().x - AI_MAX_SIGHT * CELL_SIZE) / LOCATION_SIZE,
		(vehicle.GetPos().y - AI_MAX_SIGHT * CELL_SIZE) / LOCATION_SIZE,
		(vehicle.GetPos().x + AI_MAX_SIGHT * CELL_SIZE) / LOCATION_SIZE,
		(vehicle.GetPos().y + AI_MAX_SIGHT * CELL_SIZE) / LOCATION_SIZE};

	world.grid_pickup.OverlapRect(receive, rt);
	for( auto i = receive.begin(); i != receive.end(); ++i )
	{
        ObjectList *ls = *i;
		for( auto it = ls->begin(); it != ls->end(); it = ls->next(it) )
		{
			GC_Pickup *pItem = (GC_Pickup *) ls->at(it);
			if( pItem->GetCarrier() || !pItem->GetVisible() ) 
			{
				continue;
			}

			if( (vehicle.GetPos() - pItem->GetPos()).sqr() <
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
			_pickupCurrent,
			applicants[world.net_rand() % applicants.size()]
		};
		for( int i = 0; i < 2; ++i )
		{
			if( NULL == items[i] ) continue;
			assert(items[i]->GetVisible());
			if( items[i]->GetCarrier() ) continue;
			float l = CreatePath(world, vehicle.GetPos(), items[i]->GetPos(), vehicle.GetOwner()->GetTeam(), AI_MAX_DEPTH, true, ws);
			if( l >= 0 )
			{
				AIPRIORITY p = items[i]->GetPriority(world, vehicle) - AIP_NORMAL * l / AI_MAX_DEPTH;
				if( items[i]->GetType() == _favoriteWeaponType )
				{
					if( vehicle.GetWeapon()
						&& _favoriteWeaponType != vehicle.GetWeapon()->GetType()
						&& !vehicle.GetWeapon()->GetAdvanced() )
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

void AIController::SelectFavoriteWeapon(World &world)
{
	int wcount = 0;
	for( int i = 0; i < RTTypes::Inst().GetTypeCount(); ++i )
	{
		ObjectType type = RTTypes::Inst().GetTypeByIndex(i);
		const char *name = RTTypes::Inst().GetTypeName(type);
		if( name == strstr(name, "weap_") )
		{
			if( 0 == world.net_rand() % ++wcount )
			{
				_favoriteWeaponType = type;
			}
		}
	}
}

// calculates the position of a fake target for more accurate shooting
void AIController::CalcOutstrip(World &world, vec2d origin, GC_Vehicle *target, float Vp, vec2d &fake)
{
	ASSERT_TYPE(target, GC_Vehicle);
	float Vt = target->_lv.len();
	if( Vt < Vp )
	{
		float c = target->GetDirection().x;
		float s = target->GetDirection().y;

		float x = (target->GetPos().x - origin.x) * c +
		          (target->GetPos().y - origin.y) * s;
		float y = (target->GetPos().y - origin.y) * c -
		          (target->GetPos().x - origin.x) * s;

		float fx = x + Vt * (x * Vt + sqrt(Vp*Vp * (y*y + x*x) - Vt*Vt * y*y)) / (Vp*Vp - Vt*Vt);

		fake.x = origin.x + fx * c - y * s;
		fake.y = origin.y + fx * s + y * c;
		fake.x = std::max(.0f, std::min(world._sx, fake.x));
		fake.y = std::max(.0f, std::min(world._sy, fake.y));
	}
	else
	{
		fake = origin;
	}
}

void AIController::SetL1(aiState_l1 new_state)
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

void AIController::SetL2(aiState_l2 new_state)
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

void AIController::ProcessAction(World &world, const GC_Vehicle &vehicle, const AIWEAPSETTINGS *ws)
{
	AIITEMINFO ii_item;
	FindItem(world, vehicle, ii_item, ws);

	AIITEMINFO ii_target;
	if( FindTarget(world, vehicle, ii_target, ws) )
	{
		assert(vehicle.GetWeapon());
		_target = PtrCast<GC_RigidBodyStatic>(ii_target.object);

		if( ii_target.priority > ii_item.priority )
		{
			if( CreatePath(world, vehicle.GetPos(), _target->GetPos(), vehicle.GetOwner()->GetTeam(), AI_MAX_DEPTH, false, ws) > 0 )
			{
				SmoothPath();
			}
			_pickupCurrent = NULL;
			SetL2(L2_ATTACK);
			SetL1(L1_NONE);
		}
		else
		{
			Pickup(world, vehicle, PtrCast<GC_Pickup>(ii_item.object));
		}
	}
	else
	{
		_target = NULL;

		if( ii_item.priority > AIP_NOTREQUIRED )
		{
			assert(ii_item.object);
			if( _pickupCurrent != ii_item.object )
			{
				if( CreatePath(world, vehicle.GetPos(), ii_item.object->GetPos(), vehicle.GetOwner()->GetTeam(),
				               AI_MAX_DEPTH, false, ws) > 0 )
				{
					SmoothPath();
				}
				_pickupCurrent = PtrCast<GC_Pickup>(ii_item.object);
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

bool AIController::March(World &world, const GC_Vehicle &vehicle, float x, float y)
{
    AIWEAPSETTINGS ws;
    if( vehicle.GetWeapon() )
        vehicle.GetWeapon()->SetupAI(&ws);
    if( CreatePath(world, vehicle.GetPos(), vec2d(x, y), vehicle.GetOwner()->GetTeam(), AI_MAX_DEPTH, false, &ws) > 0 )
    {
        SmoothPath();
        SetL1(L1_NONE);
        return true;
    }
	return false;
}

bool AIController::Attack(World &world, const GC_Vehicle &vehicle, GC_RigidBodyStatic *target)
{
	if( vehicle.GetWeapon() )
	{
		_target = target;
		return true;
	}
	return false;
}


bool AIController::Pickup(World &world, const GC_Vehicle &vehicle, GC_Pickup *p)
{
	assert(p);
    if( _pickupCurrent != p )
    {
        AIWEAPSETTINGS ws;
        if( vehicle.GetWeapon() )
            vehicle.GetWeapon()->SetupAI(&ws);

        if( CreatePath(world, vehicle.GetPos(), p->GetPos(), vehicle.GetOwner()->GetTeam(), AI_MAX_DEPTH, false, &ws) > 0 )
        {
            SmoothPath();
            _pickupCurrent = p;
            SetL2(L2_PICKUP);
            SetL1(L1_NONE);
            return true;
        }
    }
    else
    {
        return true;
    }
	return false;
}

void AIController::Stop()
{
	_target = NULL;
	ClearPath();
}

void AIController::SelectState(World &world, const GC_Vehicle &vehicle, const AIWEAPSETTINGS *ws)
{
	if( !_isActive )
	{
		return;
	}
	
	ProcessAction(world, vehicle, ws);

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
			vec2d t = vehicle.GetPos()
				+ world.net_vrand(sqrtf(world.net_frand(1.0f))) * (AI_MAX_SIGHT * CELL_SIZE);
			float x = std::min(std::max(.0f, t.x), world._sx);
			float y = std::min(std::max(.0f, t.y), world._sy);

			if( CreatePath(world, vehicle.GetPos(), vec2d(x, y), vehicle.GetOwner()->GetTeam(), AI_MAX_DEPTH, false, ws) > 0 )
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

void AIController::SetActive(bool active)
{
	_isActive = active;
}

void AIController::DoState(World &world, const GC_Vehicle &vehicle, VehicleState *pVehState, const AIWEAPSETTINGS *ws)
{
	if( L1_NONE != _aiState_l1 )
		return;


	vec2d brake = vehicle.GetBrakingLength();
//	float brake_len = brake.len();
	float brakeSqr = brake.sqr();

	vec2d currentDir = vehicle.GetDirection();
	vec2d currentPos = vehicle.GetPos();
//	vec2d predictedPos = GetVehicle()->GetPos() + brake;

	if( !_path.empty() )
	{
		vec2d predictedProj;
//		std::list<PathNode>::const_iterator predictedNodeIt = FindNearPathNode(predictedPos, &predictedProj, NULL);

		vec2d currentProj;
		float offset;
//		GC_Camera::GetWorldMousePos(currentPos);
		std::list<PathNode>::const_iterator currentNodeIt = FindNearPathNode(currentPos, &currentProj, &offset);

		float desiredProjOffsetLen = vehicle.GetMaxBrakingLength() * 2;//(1 + vehicle._lv.len() / vehicle.GetMaxSpeed());
		vec2d desiredProjOffset;

		std::list<PathNode>::const_iterator it = currentNodeIt;
		if( offset > 0 )
		{
			vec2d d = it->coord;
			--it;
			d -= it->coord;
			offset -= d.len();
		}
		offset += std::min((currentPos - currentProj).len(), desiredProjOffsetLen);
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
//	if( !_target && IsAttacking() )
//	{
//		FreeTarget(); // free killed target
//		ClearPath();
//	}

	if( !vehicle.GetWeapon() )
	{
		// no targets if no weapon
		_target = NULL;
		_attackList.clear();
	}


	//
	// attack the primary target if possible
	//
	if( _target && IsTargetVisible(world, vehicle, _target))
	{
		assert(vehicle.GetWeapon());

		vec2d fake = _target->GetPos();
		GC_Vehicle *enemy = PtrDynCast<GC_Vehicle>(_target);
		if( ws->bNeedOutstrip && _level > 1 && enemy )
		{
			CalcOutstrip(world, vehicle.GetPos(), enemy, ws->fProjectileSpeed, fake);
		}

		float len = (_target->GetPos() - vehicle.GetPos()).len();
		TowerTo(vehicle, pVehState, fake, len > ws->fAttackRadius_crit, ws);

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
	else if( !_attackList.empty() && IsTargetVisible(world, vehicle, _attackList.front()) )
	{
		assert(vehicle.GetWeapon());
		GC_RigidBodyStatic *target = _attackList.front();

		float len = (target->GetPos() - vehicle.GetPos()).len();
		TowerTo(vehicle, pVehState, target->GetPos(), len > ws->fAttackRadius_crit, ws);
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
			vec2d tmp = Vec2dAddDirection(vehicle.GetDirection(), vec2d(angle[i]));

			vec2d x0 = vehicle.GetPos() + tmp * vehicle.GetRadius();
			vec2d a  = brake * len[i];

			vec2d hit, norm;
			GC_Object *o = world.TraceNearest(
				world.grid_rigid_s, 
				&vehicle,
				x0,
				a,
				&hit,
				&norm
			);

			if( o )
			{
				if( 1 == i && (hit - currentPos).len() < vehicle.GetRadius() )
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

		if( min_d > 0 )
		{
			vec2d hit_dir = currentDir; // (min_hit - currentPos).Normalize();
			min_norm = (min_norm - hit_dir * (min_norm * hit_dir)).Normalize() + min_norm;
			min_norm.Normalize();
			min_norm *= 1.4142f;// sqrt(2)
			_arrivalPoint = min_hit + min_norm * vehicle.GetRadius();
//			DbgLine(min_hit, _arrivalPoint, 0xff0000ff);
		}
	}

//	DbgLine(vehicle.GetPos(), _arrivalPoint, 0x0000ffff);


	//
	// follow the path
	//

	float dst = (currentPos - _arrivalPoint).sqr();
	if( dst > CELL_SIZE*CELL_SIZE/16 )
	{
		if( _backTime <= 0 )
		{
			RotateTo(vehicle, pVehState, _arrivalPoint, dst > brakeSqr, false);
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

bool AIController::IsTargetVisible(World &world, const GC_Vehicle &vehicle, GC_RigidBodyStatic *target, GC_RigidBodyStatic** ppObstacle)
{
	assert(vehicle.GetWeapon());

	if( GC_Weap_Gauss::GetTypeStatic() == vehicle.GetWeapon()->GetType() )  // FIXME!
		return true;

	GC_RigidBodyStatic *object = (GC_RigidBodyStatic *) world.TraceNearest(
		world.grid_rigid_s,
		&vehicle,
		vehicle.GetPos(),
		target->GetPos() - vehicle.GetPos() );

	if( object && object != target )
	{
		if( ppObstacle )
            *ppObstacle = object;
		return false;
	}
	else
	{
		if( ppObstacle )
            *ppObstacle = NULL;
		return true;
	}
}

void AIController::OnRespawn(World &world, const GC_Vehicle &vehicle)
{
	_arrivalPoint = vehicle.GetPos();
//	_jobManager.RegisterMember(this);
	SelectFavoriteWeapon(world);
}

void AIController::OnDie()
{
	_pickupCurrent = NULL;
	_target = NULL;
	ClearPath();

//	_jobManager.UnregisterMember(this);
}

////////////////////////////////////////////
// debug graphics

void AIController::debug_draw(World &world)
{
	if( !_path.empty() )
	{
//		Ellipse(hdc, int(_path.front().coord.x) - 2, int(_path.front().coord.y) - 2,
//					 int(_path.front().coord.x) + 2, int(_path.front().coord.y) + 2);

		std::list<PathNode>::iterator it = _path.begin();
		for(;;)
		{
//			vec2d v = it->coord;
			if( ++it == _path.end() ) break;
//			DbgLine(v, it->coord, 0xffffffff);
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
		sprintf(s, "%d", count);
		DrawText(hdc, s, -1, &rect, DT_SINGLELINE|DT_CENTER|DT_VCENTER|DT_NOCLIP);
		++count;
	}
*/
}

///////////////////////////////////////////////////////////////////////////////
// end of file
