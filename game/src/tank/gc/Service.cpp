// Service.cpp

#include "Service.h"
#include "globals.h"
#include "GlobalListHelper.inl"
#include "World.h"

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Service)
{
	return true;
}

GC_Service::GC_Service(World &world)
  : GC_Object()
  , _memberOf(this)
{
	if( world._serviceListener )
		world._serviceListener->OnCreate(this);
}

GC_Service::GC_Service(FromFile)
  : GC_Object(FromFile())
  , _memberOf(this)
{
}

GC_Service::~GC_Service()
{
	if( g_level->_serviceListener )
		g_level->_serviceListener->OnKill(this);
}

///////////////////////////////////////////////////////////////////////////////
// end of file
