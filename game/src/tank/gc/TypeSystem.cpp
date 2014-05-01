// TypeSystem.cpp

#include "TypeSystem.h"

GC_Object* RTTypes::CreateFromFile(ObjectType type)
{
	FromFileMap::const_iterator it = _ffm.find(type);
	if( _ffm.end() == it )
		return NULL;
	return it->second();
}

GC_Object* RTTypes::CreateObject(Level &world, ObjectType type, float x, float y)
{
	assert(IsRegistered(type));
	return GetTypeInfo(type).Create(world, x, y);
}

// end of file
