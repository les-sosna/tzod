#pragma once
#include "gc/WorldEvents.h"
#include <memory>

class SoundRender;

class SoundHarness
	: ObjectListener<GC_Pickup>
	, ObjectListener<GC_ProjectileBasedWeapon>
	, ObjectListener<GC_RigidBodyStatic>
	, ObjectListener<GC_RigidBodyDynamic>
	, ObjectListener<GC_Turret>
	, ObjectListener<GC_Vehicle>
	, ObjectListener<World>
{
public:
	SoundHarness(World &world);
	~SoundHarness();
	
	void Step();
	
private:
	World &_world;
	std::unique_ptr<SoundRender> _soundRender;
	
	// ObjectListener<GC_Pickup>
	virtual void OnPickup(GC_Pickup &obj, GC_Actor &actor) override;
	virtual void OnRespawn(GC_Pickup &obj) override;
	
	// ObjectListener<GC_ProjectileBasedWeapon>
	virtual void OnShoot(GC_ProjectileBasedWeapon &obj) override;
	
	// ObjectListener<GC_RigidBodyStatic>
	virtual void OnDestroy(GC_RigidBodyStatic &obj) override;
	virtual void OnDamage(GC_RigidBodyStatic &obj, float damage, GC_Player *from) override;
	
	// ObjectListener<GC_RigidBodyDynamic>
	virtual void OnContact(vec2d pos, float np, float tp) override;
	
	// ObjectListener<GC_Turret>
	virtual void OnStateChange(GC_Turret &obj) override;

	// ObjectListener<GC_Vehicle>
	virtual void OnLight(GC_Vehicle &obj) override;

	// ObjectListener<GC_Object>
//	virtual void OnCreate(GC_Object &obj) override {}
//	virtual void OnKill(GC_Object &obj) override {}

	// ObjectListener<World>
	virtual void OnGameStarted() override {}
	virtual void OnGameFinished();
	virtual void OnGameMessage(const char *msg) override {}
};
