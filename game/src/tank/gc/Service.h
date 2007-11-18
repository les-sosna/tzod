// Service.h

#include "Object.h"

#pragma once


class GC_Service : public GC_Object
{
	DECLARE_SELF_REGISTRATION(GC_Service);
	MemberOfGlobalList<LIST_services> _memberOf;

public:
	GC_Service();
	GC_Service(FromFile);
	virtual ~GC_Service();
	virtual bool IsSaved() const { return true; }
};

///////////////////////////////////////////////////////////////////////////////
// end of file
