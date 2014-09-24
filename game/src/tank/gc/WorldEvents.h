#pragma once

class GC_Object;

struct ObjectListener
{
	virtual void OnCreate(GC_Object &obj) = 0;
	virtual void OnKill(GC_Object &obj) = 0;
};

struct MessageListener
{
    virtual void OnGameMessage(const char *msg) = 0;
};

// end of file
