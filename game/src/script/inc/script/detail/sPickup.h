#pragma once
#include <gc/WorldEvents.h>

class World;
struct lua_State;

class sPickup : ObjectListener<GC_Pickup>
{
public:
	sPickup(World &world, lua_State *L);
	~sPickup();

private:
	World &_world;
	lua_State *_L;

	// ObjectListener<GC_Pickup>
	void OnAttach(GC_Pickup &obj, GC_Vehicle &vehicle) override;
	void OnDetach(GC_Pickup &obj) override {}
	void OnRespawn(GC_Pickup &obj) override {}
	void OnDisappear(GC_Pickup &obj) override {}
};
