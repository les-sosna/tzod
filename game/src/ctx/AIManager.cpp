#include "inc/ctx/AIManager.h"
#include <ai/ai.h>
#include <gc/Player.h>
#include <gc/Vehicle.h>
#include <gc/World.h>

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

void AIManager::AssignAI(GC_Player *player, AIDiffuculty diffuculty)
{
    std::unique_ptr<AIController> ctrl(new AIController());
	ctrl->SetDifficulty(diffuculty);
	_aiControllers.emplace(player, std::move(ctrl));
}

AIManager::ControllerStateMap AIManager::ComputeAIState(World &world, float dt)
{
	ControllerStateMap result;

	unsigned int numActionable = 0;
	for (auto &ai : _aiControllers)
		if (ai.first->GetVehicle())
			numActionable++;

	if (numActionable > 0)
	{
		unsigned int favorite = world.net_rand() % numActionable;
		unsigned int index = 0;
		for (auto &ai : _aiControllers)
		{
			if (auto vehicle = ai.first->GetVehicle())
			{
				VehicleState vs;
				ai.second->ReadControllerState(world, dt, *vehicle, vs, index == favorite);
				result.insert(std::make_pair(vehicle->GetId(), vs));
				index++;
			}
		}
	}
	return result;
}

void AIManager::GetControllers(std::vector<const AIController*> &controllers) const
{
	controllers.clear();
	controllers.reserve(_aiControllers.size());
	for (auto &ai : _aiControllers)
	{
		controllers.push_back(ai.second.get());
	}
}

void AIManager::OnRespawn(GC_Player &obj, GC_Vehicle &vehicle)
{
	auto it = _aiControllers.find(&obj);
	if (_aiControllers.end() != it)
		it->second->OnRespawn(_world, vehicle);
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

