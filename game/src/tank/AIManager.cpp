#include "AIManager.h"
#include "ai.h"
#include "gc/Player.h"
#include "gc/Vehicle.h"
#include "gc/World.h"

AIManager::AIManager(World &world)
	: _world(world)
{
	_world.eGC_Player.AddListener(*this);
	_world.eWorld.AddListener(*this);
}

AIManager::~AIManager()
{
	_world.eWorld.RemoveListener(*this);
	_world.eGC_Player.RemoveListener(*this);
}

void AIManager::AssignAI(GC_Player *player, std::string profile)
{
    std::unique_ptr<AIController> ctrl(new AIController());
//    ctrl->SetProfile(profile.c_str());
    _aiControllers[player] = std::make_pair(std::move(profile), std::move(ctrl));
}

void AIManager::FreeAI(GC_Player *player)
{
	
}

AIManager::ControllerStateMap AIManager::ComputeAIState(World &world, float dt)
{
	ControllerStateMap result;
	for (auto &ai: _aiControllers)
	{
		if( auto vehicle = ai.first->GetVehicle() )
		{
			VehicleState vs;
			ai.second.second->ReadControllerState(world, dt, *vehicle, vs);
			result.insert(std::make_pair(vehicle->GetId(), vs));
		}
	}
	return std::move(result);
}

void AIManager::OnRespawn(GC_Player &obj, GC_Vehicle &vehicle)
{
	auto it = _aiControllers.find(&obj);
	if (_aiControllers.end() != it)
		it->second.second->OnRespawn(_world, vehicle);
}

void AIManager::OnDie(GC_Player &obj)
{
}

void AIManager::OnKill(GC_Object &obj)
{
	if (GC_Player::GetTypeStatic() == obj.GetType())
	{
		_aiControllers.erase(static_cast<GC_Player *>(&obj));
	}
}

