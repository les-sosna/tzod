#pragma once
#include "gc/WorldEvents.h"

class World;
struct GameListener;

class Deathmatch
	: private ObjectListener<GC_RigidBodyStatic>
{
public:
	Deathmatch(World &world, GameListener &gameListener);
	~Deathmatch();
	
private:
	World &_world;
	GameListener &_gameListener;
	
	// ObjectListener<GC_RigidBodyStatic>
	virtual void OnDestroy(GC_RigidBodyStatic &obj, const DamageDesc &dd) override;
	virtual void OnDamage(GC_RigidBodyStatic &obj, const DamageDesc &dd) override {}
};
