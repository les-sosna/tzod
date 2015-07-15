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
	virtual void OnAttach(GC_Pickup &obj, GC_Vehicle &vehicle) override;
	virtual void OnDetach(GC_Pickup &obj) override {}
	virtual void OnRespawn(GC_Pickup &obj) override {}
	virtual void OnDisappear(GC_Pickup &obj) override {}
};
