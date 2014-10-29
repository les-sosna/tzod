#pragma once

#include <math/MyMath.h>

template <class T> struct ObjectListener;

class GC_Pickup;
class GC_Vehicle;
template<> struct ObjectListener<GC_Pickup>
{
	virtual void OnAttach(GC_Pickup &obj, GC_Vehicle &vehicle) = 0;
	virtual void OnDetach(GC_Pickup &obj) = 0;
	virtual void OnRespawn(GC_Pickup &obj) = 0;
	virtual void OnDisappear(GC_Pickup &obj) = 0;
};

struct DamageDesc;
class GC_pu_Shield;
class GC_Vehicle;
template<> struct ObjectListener<GC_pu_Shield>
{
	virtual void OnOwnerDamage(GC_pu_Shield &obj, DamageDesc &dd) = 0;
	virtual void OnExpiring(GC_pu_Shield &obj) = 0;
};

class GC_Player;
class GC_Vehicle;
template<> struct ObjectListener<GC_Player>
{
	virtual void OnRespawn(GC_Player &obj, GC_Vehicle &vehicle) = 0;
	virtual void OnDie(GC_Player &obj) = 0;
};

class GC_Projectile;
class GC_RigidBodyStatic;
template<> struct ObjectListener<GC_Projectile>
{
	virtual void OnHit(GC_Projectile &obj, GC_RigidBodyStatic &target, vec2d hit) = 0;
};

class GC_ProjectileBasedWeapon;
template<> struct ObjectListener<GC_ProjectileBasedWeapon>
{
	virtual void OnShoot(GC_ProjectileBasedWeapon &obj) = 0;
};

class GC_Trigger;
class GC_Vehicle;
template<> struct ObjectListener<GC_Trigger>
{
	virtual void OnEnter(GC_Trigger &obj, GC_Vehicle &vehicle) = 0;
	virtual void OnLeave(GC_Trigger &obj) = 0;
};

class GC_Player;
class GC_RigidBodyStatic;
template<> struct ObjectListener<GC_RigidBodyStatic>
{
	virtual void OnDestroy(GC_RigidBodyStatic &obj) = 0;
	virtual void OnDamage(GC_RigidBodyStatic &obj, float damage, GC_Player *from) = 0;
};

class GC_RigidBodyDynamic;
template<> struct ObjectListener<GC_RigidBodyDynamic>
{
	virtual void OnContact(vec2d pos, float np, float tp) = 0;
};

class GC_Vehicle;
template<> struct ObjectListener<GC_Vehicle>
{
	virtual void OnLight(GC_Vehicle &obj) = 0;
};

class GC_Turret;
template<> struct ObjectListener<GC_Turret>
{
	virtual void OnShoot(GC_Turret &obj) = 0;
	virtual void OnStateChange(GC_Turret &obj) = 0;
	virtual void OnRotationStateChange(GC_Turret &obj) = 0;
};

class GC_Object;
class World;
template<> struct ObjectListener<World>
{
	virtual void OnGameMessage(const char *msg) = 0;
	virtual void OnKill(GC_Object &obj) = 0;
	virtual void OnNewObject(GC_Object &obj) = 0;

	// TODO: these functions should not be here
	virtual void OnGameStarted() = 0;
	virtual void OnGameFinished() = 0;
};

// end of file
