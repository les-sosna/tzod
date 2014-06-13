#include "WorldController.h"
#include "gc/World.h"
#include "gc/Player.h"
#include "gc/Vehicle.h"


WorldController::WorldController(World &world)
	: _world(world)
{
}

void WorldController::SendControllerStates(ControllerStateMap stateMap)
{
	for (auto playerState: stateMap)
	{
		auto player = static_cast<GC_Player*>(_world.GetList(LIST_players).at(playerState.first));
		if (auto vehicle = player->GetVehicle())
		{
			vehicle->SetControllerState(playerState.second);
		}
	}
}

