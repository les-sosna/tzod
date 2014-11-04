#pragma once
#include "gc/WorldEvents.h"
#include <memory>
#include <unordered_map>

class SoundRender;
class GC_Weapon;
struct Sound;

class SoundHarness
	: ObjectListener<GC_Pickup>
	, ObjectListener<GC_Projectile>
	, ObjectListener<GC_ProjectileBasedWeapon>
	, ObjectListener<GC_pu_Shield>
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
	std::unordered_map<GC_Weapon*, std::unique_ptr<Sound>> _weaponRotate;
	std::unordered_map<GC_Weapon*, std::unique_ptr<Sound>> _weaponFire;
	std::unordered_map<GC_Turret*, std::unique_ptr<Sound>> _turrets;
	
	// ObjectListener<GC_Pickup>
	virtual void OnAttach(GC_Pickup &obj, GC_Vehicle &vehicle) override;
	virtual void OnDetach(GC_Pickup &obj) override;
	virtual void OnRespawn(GC_Pickup &obj) override;
	virtual void OnDisappear(GC_Pickup &obj) override;

	// ObjectListener<GC_Projectile>
	virtual void OnHit(GC_Projectile &obj, GC_RigidBodyStatic &target, vec2d hit) override;
	
	// ObjectListener<GC_ProjectileBasedWeapon>
	virtual void OnShoot(GC_ProjectileBasedWeapon &obj) override;
	
	// ObjectListener<GC_pu_Shield>
	virtual void OnOwnerDamage(GC_pu_Shield &obj, DamageDesc &dd) override;
	virtual void OnExpiring(GC_pu_Shield &obj) override;
	
	// ObjectListener<GC_RigidBodyStatic>
	virtual void OnDestroy(GC_RigidBodyStatic &obj) override;
	virtual void OnDamage(GC_RigidBodyStatic &obj, float damage, GC_Player *from) override;
	
	// ObjectListener<GC_RigidBodyDynamic>
	virtual void OnContact(vec2d pos, float np, float tp) override;
	
	// ObjectListener<GC_Turret>
	virtual void OnShoot(GC_Turret &obj) override;
	virtual void OnStateChange(GC_Turret &obj) override;
	virtual void OnRotationStateChange(GC_Turret &obj) override;

	// ObjectListener<GC_Vehicle>
	virtual void OnLight(GC_Vehicle &obj) override;

	// ObjectListener<World>
	virtual void OnGameStarted() override {}
	virtual void OnGameFinished();
	virtual void OnGameMessage(const char *msg) override {}
	virtual void OnKill(GC_Object &) override {}
	virtual void OnNewObject(GC_Object &obj) override;
};
