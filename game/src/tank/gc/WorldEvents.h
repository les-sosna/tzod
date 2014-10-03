#pragma once

template <class T> struct ObjectListener;

class GC_Object;
template<> struct ObjectListener<GC_Object>
{
	virtual void OnCreate(GC_Object &obj) = 0;
	virtual void OnKill(GC_Object &obj) = 0;
};

class GC_Actor;
class GC_Pickup;
template<> struct ObjectListener<GC_Pickup>
{
	virtual void OnPickup(GC_Pickup &obj, GC_Actor &actor) = 0;
	virtual void OnRespawn(GC_Pickup &obj) = 0;
};

class GC_Player;
class GC_Vehicle;
template<> struct ObjectListener<GC_Player>
{
	virtual void OnRespawn(GC_Player &obj, GC_Vehicle &vehicle) = 0;
	virtual void OnDie(GC_Player &obj) = 0;
};

class GC_Service;
template<> struct ObjectListener<GC_Service> : ObjectListener<GC_Object> {};

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

class GC_Vehicle;
template<> struct ObjectListener<GC_Vehicle>
{
	virtual void OnLight(GC_Vehicle &obj) = 0;
};

class GC_Turret;
template<> struct ObjectListener<GC_Turret>
{
	virtual void OnStateChange(GC_Turret &obj) = 0;
};

class World;
template<> struct ObjectListener<World>
{
	virtual void OnGameStarted() = 0;
	virtual void OnGameFinished() = 0;
    virtual void OnGameMessage(const char *msg) = 0;
};

// end of file
