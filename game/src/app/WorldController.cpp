#include "inc/app/WorldController.h"
#include <gc/Player.h>
#include <gc/Vehicle.h>
#include <gc/World.h>
#include <gc/Macros.h>


WorldController::WorldController(World &world)
	: _world(world)
{
}

std::vector<GC_Player*> WorldController::GetLocalPlayers()
{
	std::vector<GC_Player*> players;
	FOREACH(_world.GetList(LIST_players), GC_Player, player)
	{
		// TODO: differentiate local and remote players
		if (player->GetIsHuman())
		{
			players.push_back(player);
		}
	}
	return players;
}

std::vector<GC_Player*> WorldController::GetAIPlayers()
{
	std::vector<GC_Player*> players;
	FOREACH(_world.GetList(LIST_players), GC_Player, player)
	{
		if (!player->GetIsHuman())
		{
			players.push_back(player);
		}
	}
	return players;
}

void WorldController::SendControllerStates(ControllerStateMap stateMap)
{
	for (auto playerState: stateMap)
	{
		auto vehicle = static_cast<GC_Vehicle*>(_world.GetList(LIST_objects).at(playerState.first));
		vehicle->SetControllerState(playerState.second);
	}
}
