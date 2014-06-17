#include "AIManager.h"
#include "ai.h"
#include "gc/Player.h"
#include "gc/Vehicle.h"

AIManager::AIManager()
{
}

AIManager::~AIManager()
{
	assert(_aiControllers.empty());
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
