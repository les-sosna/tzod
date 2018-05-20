#include "inc/ai/ai.h"
#include "DrivingAgent.h"
#include "ShootingAgent.h"
#include <gc/Pickup.h>
#include <gc/Player.h>
#include <gc/TypeSystem.h>
#include <gc/Turrets.h>
#include <gc/Vehicle.h>
#include <gc/Weapons.h>
#include <gc/World.h>
#include <gc/WorldCfg.h>
#include <gc/Macros.h>
#include <gc/SaveFile.h>

AIController::AIController()
  : _drivingAgent(new DrivingAgent())
  , _shootingAgent(new ShootingAgent())
  , _favoriteWeaponType(INVALID_OBJECT_TYPE)
  , _difficulty(2)
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
	_drivingAgent->Serialize(f);
	_shootingAgent->Serialize(f);

	f.Serialize(_difficulty);
	f.Serialize(_aiState_l1);
	f.Serialize(_aiState_l2);
	f.Serialize(_favoriteWeaponType);
	f.Serialize(_pickupCurrent);
	f.Serialize(_target);
	f.Serialize(_isActive);
}

void AIController::ReadControllerState(World &world, float dt, const GC_Vehicle &vehicle, VehicleState &outVehicleState, bool allowExtraCalc)
{
	memset(&outVehicleState, 0, sizeof(VehicleState));

	if( _pickupCurrent )
	{
		if( !_pickupCurrent->GetVisible() )
		{
			_pickupCurrent = nullptr;
		}
		else if( (_pickupCurrent->GetPos() - vehicle.GetPos()).sqr() <
		         std::pow(_pickupCurrent->GetRadius() + vehicle.GetRadius(), 2) )
		{
			outVehicleState.pickup = true;
		}
	}

	AIWEAPSETTINGS weapSettings;
	if( vehicle.GetWeapon() )
		vehicle.GetWeapon()->SetupAI(&weapSettings);


	// take decision
	if( allowExtraCalc )
		SelectState(world, vehicle, vehicle.GetWeapon() ? &weapSettings : nullptr);


	if (L1_NONE == _aiState_l1)
	{
		_drivingAgent->ComputeState(world, vehicle, dt, outVehicleState);


		// check if the primary target is still alive
		//	if( !_target && IsAttacking() )
		//	{
		//		FreeTarget(); // free killed target
		//		ClearPath();
		//	}

		if (!vehicle.GetWeapon())
		{
			// no targets if no weapon
			_target = nullptr;
			_drivingAgent->_attackList.clear();
		}


		if (_target && IsTargetVisible(world, vehicle, _target))
		{
			// attack the primary target
			_shootingAgent->AttackTarget(world, vehicle, *_target, dt, outVehicleState);
			_drivingAgent->StayAway(_target->GetPos(), weapSettings.fAttackRadius_min);
		}
		else
		{
			_drivingAgent->StayAway({}, 0);

			if (!_drivingAgent->_attackList.empty() && IsTargetVisible(world, vehicle, _drivingAgent->_attackList.front()))
			{
				// attack secondary targets
				_shootingAgent->AttackTarget(world, vehicle, *_drivingAgent->_attackList.front(), dt, outVehicleState);
			}
		}

//		DbgLine(vehicle.GetPos(), _arrivalPoint, 0x0000ffff);
	}

	// headlight control
	switch( _difficulty )
	{
	case 0:
	case 1:
		outVehicleState.light = true;
		break;
	case 2:
	case 3:
		outVehicleState.light = (nullptr != _target);
		break;
	default:
		outVehicleState.light = false;
	}
}

const std::vector<vec2d>& AIController::GetPath() const
{
	return _drivingAgent->GetPath();
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
	GC_Vehicle *pOptTarget = nullptr;

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
				(AI_MAX_SIGHT * WORLD_BLOCK_SIZE) * (AI_MAX_SIGHT * WORLD_BLOCK_SIZE) )
			{
				GC_RigidBodyStatic *pObstacle = static_cast<GC_RigidBodyStatic*>(
					world.TraceNearest(world.grid_rigid_s, &vehicle,
					vehicle.GetPos(), object->GetPos() - vehicle.GetPos()) );

				TargetDesc td;
				td.target = object;
				td.bIsVisible = (nullptr == pObstacle || pObstacle == object);

				targets.push_back(td);
			}
		}
	}

	for( size_t i = 0; i < targets.size(); ++i )
	{
		float l;
		if( targets[i].bIsVisible )
			l = (targets[i].target->GetPos() - vehicle.GetPos()).len() / WORLD_BLOCK_SIZE;
		else
			l = _drivingAgent->CreatePath(world, vehicle.GetPos(), targets[i].target->GetPos(), vehicle.GetOwner()->GetTeam(), AI_MAX_DEPTH, true, ws);

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
		(vehicle.GetPos().x - AI_MAX_SIGHT * WORLD_BLOCK_SIZE) / WORLD_LOCATION_SIZE,
		(vehicle.GetPos().y - AI_MAX_SIGHT * WORLD_BLOCK_SIZE) / WORLD_LOCATION_SIZE,
		(vehicle.GetPos().x + AI_MAX_SIGHT * WORLD_BLOCK_SIZE) / WORLD_LOCATION_SIZE,
		(vehicle.GetPos().y + AI_MAX_SIGHT * WORLD_BLOCK_SIZE) / WORLD_LOCATION_SIZE};

	world.grid_pickup.OverlapRect(receive, rt);
	for( auto i = receive.begin(); i != receive.end(); ++i )
	{
        ObjectList *ls = *i;
		for( auto it = ls->begin(); it != ls->end(); it = ls->next(it) )
		{
			GC_Pickup *pItem = (GC_Pickup *) ls->at(it);
			if( pItem->GetAttached() || !pItem->GetVisible() )
			{
				continue;
			}

			if( (vehicle.GetPos() - pItem->GetPos()).sqr() <
				(AI_MAX_SIGHT * WORLD_BLOCK_SIZE) * (AI_MAX_SIGHT * WORLD_BLOCK_SIZE) )
			{
				applicants.push_back(pItem);
			}
		}
	}


	AIPRIORITY optimal  = AIP_NOTREQUIRED;
	GC_Pickup *pOptItem = nullptr;

	if( !applicants.empty() )
	{
		GC_Pickup *items[2] = {
			_pickupCurrent,
			applicants[world.net_rand() % applicants.size()]
		};
		for( int i = 0; i < 2; ++i )
		{
			if( nullptr == items[i] ) continue;
			assert(items[i]->GetVisible());
			if( items[i]->GetAttached() ) continue;
			float l = _drivingAgent->CreatePath(world, vehicle.GetPos(), items[i]->GetPos(), vehicle.GetOwner()->GetTeam(), AI_MAX_DEPTH, true, ws);
			// TODO: path cost should be non linear function of the length
			if( l >= 0 )
			{
				AIPRIORITY p = items[i]->GetPriority(world, vehicle) - AIP_NORMAL * l / AI_MAX_DEPTH;
				if( items[i]->GetType() == _favoriteWeaponType )
				{
					if( vehicle.GetWeapon()
						&& _favoriteWeaponType != vehicle.GetWeapon()->GetType()
						&& !vehicle.GetWeapon()->GetBooster() )
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
	for( unsigned int i = 0; i < RTTypes::Inst().GetTypeCount(); ++i )
	{
		ObjectType type = RTTypes::Inst().GetTypeByIndex(i);
		const char *name = RTTypes::Inst().GetTypeInfo(type).name;
		if( name == std::strstr(name, "weap_") )
		{
			if( 0 == world.net_rand() % ++wcount )
			{
				_favoriteWeaponType = type;
			}
		}
	}
}

void AIController::SetL1(aiState_l1 new_state)
{
#ifndef NDEBUG
	if( _aiState_l1 == new_state )
		return;

	switch (new_state)
	{
	case L1_PATH_END:
//		REPORT("AI switch to L1_PATH_END\n");
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
#ifndef NDEBUG
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
			if(_drivingAgent->CreatePath(world, vehicle.GetPos(), _target->GetPos(), vehicle.GetOwner()->GetTeam(), AI_MAX_DEPTH, false, ws) > 0 )
			{
				_drivingAgent->SmoothPath();
			}
			_pickupCurrent = nullptr;
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
		_target = nullptr;

		if( ii_item.priority > AIP_NOTREQUIRED )
		{
			assert(ii_item.object);
			if( _pickupCurrent != ii_item.object )
			{
				if(_drivingAgent->CreatePath(world, vehicle.GetPos(), ii_item.object->GetPos(), vehicle.GetOwner()->GetTeam(), AI_MAX_DEPTH, false, ws) > 0 )
				{
					_drivingAgent->SmoothPath();
				}
				_pickupCurrent = PtrCast<GC_Pickup>(ii_item.object);
			}
			SetL2(L2_PICKUP);
			SetL1(L1_NONE);
		}
		else
		{
			_pickupCurrent = nullptr;
			SetL2(L2_PATH_SELECT);
		}
	}
}

void AIController::SetLevel(int level)
{
	_difficulty = level;
	_drivingAgent->SetAttackFriendlyTurrets(level == 0); // be totally stupid
}

bool AIController::March(World &world, const GC_Vehicle &vehicle, float x, float y)
{
    AIWEAPSETTINGS ws;
    if( vehicle.GetWeapon() )
        vehicle.GetWeapon()->SetupAI(&ws);
	if (_drivingAgent->CreatePath(world, vehicle.GetPos(), { x, y }, vehicle.GetOwner()->GetTeam(), AI_MAX_DEPTH, false, &ws) > 0)
    {
		_drivingAgent->SmoothPath();
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

        if(_drivingAgent->CreatePath(world, vehicle.GetPos(), p->GetPos(), vehicle.GetOwner()->GetTeam(), AI_MAX_DEPTH, false, &ws) > 0 )
        {
			_drivingAgent->SmoothPath();
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
	_target = nullptr;
	_drivingAgent->ClearPath();
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
		break;
	case L2_ATTACK:
		assert(_target);
		break;
	case L2_PATH_SELECT:
		assert(nullptr == _target);
		if( !_drivingAgent->HasPath() )
		{
			vec2d t = vehicle.GetPos() + world.net_vrand(sqrtf(world.net_frand(1.0f))) * (AI_MAX_SIGHT * WORLD_BLOCK_SIZE);
			t = Vec2dClamp(t, world.GetBounds());

			if (_drivingAgent->CreatePath(world, vehicle.GetPos(), t, vehicle.GetOwner()->GetTeam(), AI_MAX_DEPTH, false, ws) > 0)
			{
				_drivingAgent->SmoothPath();
				SetL1(L1_NONE);
			}
			else
			{
				_drivingAgent->ClearPath();
			}
		}
		break;
	}
}

void AIController::SetActive(bool active)
{
	_isActive = active;
}

bool AIController::IsTargetVisible(const World &world, const GC_Vehicle &vehicle, GC_RigidBodyStatic *target, GC_RigidBodyStatic** ppObstacle)
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
			*ppObstacle = nullptr;
		return true;
	}
}

void AIController::OnRespawn(World &world, const GC_Vehicle &vehicle)
{
//	_drivingAgent->_arrivalPoint = vehicle.GetPos();
//	_jobManager.RegisterMember(this);
	SelectFavoriteWeapon(world);
}

void AIController::OnDie()
{
	_pickupCurrent = nullptr;
	_target = nullptr;
	_drivingAgent->ClearPath();
}
