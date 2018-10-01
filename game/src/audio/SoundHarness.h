#pragma once
#include <gc/WorldEvents.h>
#include <memory>
#include <unordered_map>

class GC_Actor;
class GC_Weapon;
struct GameContextBase;
struct Gameplay;
struct Sound;
struct SoundRender;

class SoundHarness final
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
	SoundHarness(SoundRender &soundRender, GameContextBase &gameContext, const Gameplay *gameplay);
	~SoundHarness();

	void SetListenerPos(vec2d pos);
	void Step();

private:
	GameContextBase &_gameContext;
	const Gameplay *_gameplay;
		SoundRender &_soundRender;
	int _secondsLeftLastStep = -1;
	std::unordered_map<const GC_Actor*, std::unique_ptr<Sound>> _attached;
	std::unordered_map<const GC_Turret*, std::unique_ptr<Sound>> _turretFire;
	std::unordered_map<const GC_Turret*, std::unique_ptr<Sound>> _turretRotate;
	std::unordered_map<const GC_Vehicle*, std::unique_ptr<Sound>> _vehicleMove;
	std::unordered_map<const GC_Weapon*, std::unique_ptr<Sound>> _weaponFire;
	std::unordered_map<const GC_Weapon*, std::unique_ptr<Sound>> _weaponRotate;

	// ObjectListener<GC_Pickup>
	void OnAttach(GC_Pickup &obj, GC_Vehicle &vehicle, bool pickedUp) override;
	void OnDetach(GC_Pickup &obj) override;
	void OnRespawn(GC_Pickup &obj) override;
	void OnDisappear(GC_Pickup &obj) override;

	// ObjectListener<GC_Projectile>
	void OnHit(GC_Projectile &obj, GC_RigidBodyStatic &target, vec2d hit) override;

	// ObjectListener<GC_ProjectileBasedWeapon>
	void OnShoot(GC_ProjectileBasedWeapon &obj) override;

	// ObjectListener<GC_pu_Shield>
	void OnOwnerDamage(GC_pu_Shield &obj, DamageDesc &dd) override;
	void OnExpiring(GC_pu_Shield &obj) override;

	// ObjectListener<GC_RigidBodyStatic>
	void OnDestroy(GC_RigidBodyStatic &obj, const DamageDesc &dd) override;
	void OnDamage(GC_RigidBodyStatic &obj, const DamageDesc &dd) override;

	// ObjectListener<GC_RigidBodyDynamic>
	void OnContact(vec2d pos, float np, float tp) override;

	// ObjectListener<GC_Turret>
	void OnShoot(GC_Turret &obj) override;
	void OnStateChange(GC_Turret &obj) override;
	void OnRotationStateChange(GC_Turret &obj) override;
	void OnFireStateChange(GC_Turret &obj) override;

	// ObjectListener<GC_Vehicle>
	void OnLight(GC_Vehicle &obj) override;

	// ObjectListener<World>
	void OnGameStarted() override {}
	void OnGameFinished() override;
	void OnKill(GC_Object &obj) override;
	void OnNewObject(GC_Object &obj) override;
};
