// TypeSystem.cpp

#include "TypeSystem.h"
#include "Object.h"

GC_Object* RTTypes::CreateFromFile(World &world, ObjectType type)
{
	FromFileMap::const_iterator it = _ffm.find(type);
	if( _ffm.end() == it )
		return NULL;
    GC_Object *obj = it->second(world);
	return obj;
}

GC_Object* RTTypes::CreateObject(World &world, ObjectType type, float x, float y)
{
	assert(IsRegistered(type));
    GC_Object *obj = GetTypeInfo(type).Create(world, x, y);
	return obj;
}

// end of file
