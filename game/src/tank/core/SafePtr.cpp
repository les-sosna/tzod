// SafePtr.cpp

#include "stdafx.h"
#include "SafePtr.h"

///////////////////////////////////////////////////////////////////////////////
// RefCounted class implementation

#ifndef NDEBUG
RefCounted::tracker RefCounted::_tracker;
#endif

RefCounted::RefCounted()
  : _refCount(0)
{
#ifndef NDEBUG
    _tracker._objects.insert(this);
#endif
}

RefCounted::~RefCounted()
{
#ifndef NDEBUG
    _tracker._objects.erase(this);
#endif
}


// end of file
