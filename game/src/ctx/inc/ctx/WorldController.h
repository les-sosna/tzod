#pragma once
#include <gc/detail/PtrList.h> // fixme: detail
#include <gc/VehicleState.h>
#include <map>
#include <vector>

class World;
class GC_Object;
class GC_Player;

class WorldController
{
public:
	explicit WorldController(World &world);

	std::vector<GC_Player*> GetLocalPlayers();
	std::vector<GC_Player*> GetAIPlayers();

	typedef std::map<PtrList<GC_Object>::id_type, VehicleState> ControllerStateMap;
	void SendControllerStates(ControllerStateMap stateMap);

private:
	World &_world;
};
