// Service.h

#pragma once
#include "Object.h"

#define GC_FLAG_SERVICE_   GC_FLAG_OBJECT_

class GC_Service : public GC_Object
{
    typedef GC_Object base;

public:
    DECLARE_MEMBER_OF(LIST_services);
};

// end of file
