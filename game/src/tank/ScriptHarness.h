#pragma once
#include "gc/WorldEvents.h"

class World;
struct lua_State;

class ScriptHarness
	: ObjectListener<GC_Trigger>
	, ObjectListener<GC_RigidBodyStatic>
	, ObjectListener<GC_Pickup>
	, ObjectListener<GC_Player>
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
	
	// ObjectListener<GC_Pickup>
	virtual void OnPickup(GC_Pickup &obj, GC_Actor &actor) override;
	
	// ObjectListener<GC_Player>
	virtual void OnRespawn(GC_Player &obj, GC_Vehicle &vehicle) override;
	virtual void OnDie(GC_Player &obj) override;

	// ObjectListener<GC_Object>
	virtual void OnCreate(GC_Object &) override {}
	virtual void OnKill(GC_Object &) override {}
};
