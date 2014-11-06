#pragma once
#include "gc/WorldEvents.h"

class World;

class Deathmatch
	: private ObjectListener<GC_RigidBodyStatic>
{
public:
	Deathmatch(World &world);
	~Deathmatch();
	
private:
	World &_world;
	
	// ObjectListener<GC_RigidBodyStatic>
	virtual void OnDestroy(GC_RigidBodyStatic &obj, const DamageDesc &dd) override;
	virtual void OnDamage(GC_RigidBodyStatic &obj, const DamageDesc &dd) override {}
};
