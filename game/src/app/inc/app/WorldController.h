#pragma once
#include <gc/detail/PtrList.h> // fixme: detail
#include <gc/VehicleState.h>
#include <map>

class World;
class GC_Object;

class WorldController
{
public:
	WorldController(World &world);

	typedef std::map<PtrList<GC_Object>::id_type, VehicleState> ControllerStateMap;
	void SendControllerStates(ControllerStateMap stateMap);

private:
	World &_world;
};
