#pragma once
#include "gc/WorldEvents.h"

class World;
struct lua_State;

class sRigidBodyStatic : ObjectListener<GC_RigidBodyStatic>
{
public:
	sRigidBodyStatic(World &world, lua_State *L);
	~sRigidBodyStatic();
	
private:
	World &_world;
	lua_State *_L;
	
	// ObjectListener<GC_RigidBodyStatic>
	virtual void OnDestroy(GC_RigidBodyStatic &obj, const DamageDesc &dd) override;
	virtual void OnDamage(GC_RigidBodyStatic &obj, const DamageDesc &dd) override;
};
