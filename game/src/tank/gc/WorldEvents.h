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


struct MessageListener
{
    virtual void OnGameMessage(const char *msg) = 0;
};

// end of file
