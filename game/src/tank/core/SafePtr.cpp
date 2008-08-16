// SafePtr.cpp

#include "stdafx.h"
#include "SafePtr.h"

///////////////////////////////////////////////////////////////////////////////
// RefCounted class implementation

#ifdef _DEBUG
RefCounted::tracker RefCounted::_tracker;
#endif

RefCounted::RefCounted()
  : _refCount(0)
{
#ifdef _DEBUG
    ++_tracker._count;
#endif
}

RefCounted::~RefCounted()
{
#ifdef _DEBUG
    --_tracker._count;
#endif
}


// end of file
