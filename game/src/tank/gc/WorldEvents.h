#pragma once

template <class T> struct ObjectListener;

class GC_Object;
template<> struct ObjectListener<GC_Object>
{
	virtual void OnCreate(GC_Object &obj) = 0;
	virtual void OnKill(GC_Object &obj) = 0;
};

class GC_Player;
template<> struct ObjectListener<GC_Player> : ObjectListener<GC_Object> {};

class GC_Service;
template<> struct ObjectListener<GC_Service> : ObjectListener<GC_Object> {};

class GC_Trigger;
class GC_Vehicle;
template<> struct ObjectListener<GC_Trigger>
{
	virtual void OnEnter(GC_Trigger &obj, GC_Vehicle &vehicle) = 0;
	virtual void OnLeave(GC_Trigger &obj) = 0;
};

class GC_Actor;
class GC_RigidBodyStatic;
template<> struct ObjectListener<GC_RigidBodyStatic>
{
	virtual void OnDestroy(GC_RigidBodyStatic &obj) = 0;
	virtual void OnDamage(GC_RigidBodyStatic &obj, GC_Actor *from) = 0;
};


struct MessageListener
{
    virtual void OnGameMessage(const char *msg) = 0;
};

// end of file
