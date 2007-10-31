// Trigger.cpp

#include "stdafx.h"
#include "Trigger.h"
#include "level.h"

IMPLEMENT_SELF_REGISTRATION(GC_Trigger)
{
	ED_ITEM( "trigger", "Триггер" );
	return true;
}


GC_Trigger::GC_Trigger(float x, float y) : GC_2dSprite()
{
	SetTexture("editor_trigger");
	MoveTo(vec2d(x, y));
	SetZ(Z_WOOD);
	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FIXED);
}

GC_Trigger::GC_Trigger(FromFile) : GC_2dSprite(FromFile())
{
}

GC_Trigger::~GC_Trigger()
{
}



// end of file
