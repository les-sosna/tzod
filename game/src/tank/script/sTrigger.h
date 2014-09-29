#pragma once
#include "gc/WorldEvents.h"

class World;
struct lua_State;

class sTrigger : ObjectListener<GC_Trigger>
{
public:
	sTrigger(World &world, lua_State *L);
	~sTrigger();
	
private:
	World &_world;
	lua_State *_L;
	
	// ObjectListener<GC_Trigger>
	virtual void OnEnter(GC_Trigger &obj, GC_Vehicle &vehicle) override;
	virtual void OnLeave(GC_Trigger &obj) override;
};
