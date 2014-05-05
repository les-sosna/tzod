// Service.h

#include "Object.h"

#pragma once

#define GC_FLAG_SERVICE_   GC_FLAG_OBJECT_


class GC_Service : public GC_Object
{
	DECLARE_SELF_REGISTRATION(GC_Service);
	MemberOfGlobalList<LIST_services> _memberOf;

public:
	GC_Service(World &world);
	GC_Service(FromFile);
	virtual ~GC_Service();
};

///////////////////////////////////////////////////////////////////////////////
// end of file
