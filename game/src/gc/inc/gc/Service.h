#pragma once
#include "Object.h"

#define GC_FLAG_SERVICE_   GC_FLAG_OBJECT_

class GC_Service : public GC_Object
{
    DECLARE_LIST_MEMBER(override);
    typedef GC_Object base;
};
