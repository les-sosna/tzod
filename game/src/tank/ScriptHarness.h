#pragma once
#include "gc/WorldEvents.h"

class World;
struct lua_State;

class ScriptHarness
	: ObjectListener<GC_Trigger>
	, ObjectListener<GC_RigidBodyStatic>
{
public:
	ScriptHarness(World &world, lua_State *L);
	~ScriptHarness();
	
private:
	World &_world;
	lua_State *_L;
	
	// ObjectListener<GC_Trigger>
	virtual void OnEnter(GC_Trigger &obj, GC_Vehicle &vehicle) override;
	virtual void OnLeave(GC_Trigger &obj) override;
	
	// ObjectListener<GC_RigidBodyStatic>
	virtual void OnDestroy(GC_RigidBodyStatic &obj) override;
	virtual void OnDamage(GC_RigidBodyStatic &obj, GC_Actor *from) override;
};
