// ObjectListener.h

#pragma once

class GC_Object;

interface ObjectListener
{
	virtual void OnCreate(GC_Object *obj) = 0;
	virtual void OnKill(GC_Object *obj) = 0;
};

// end of file
