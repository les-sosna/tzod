#include "WorldController.h"
#include "gc/World.h"
#include "gc/Vehicle.h"


WorldController::WorldController(World &world)
	: _world(world)
{
}

void WorldController::SendControllerStates(ControllerStateMap stateMap)
{
	for (auto playerState: stateMap)
	{
		auto vehicle = static_cast<GC_Vehicle*>(_world.GetList(LIST_objects).at(playerState.first));
		vehicle->SetControllerState(playerState.second);
	}
}

