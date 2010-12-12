// Service.cpp

#include "stdafx.h"

#include "Service.h"
#include "Level.h"

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Service)
{
	return true;
}

GC_Service::GC_Service()
  : GC_Object()
  , _memberOf(this)
{
	if( g_level->_serviceListener )
		g_level->_serviceListener->OnCreate(this);
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
