#include "inc/gc/TypeSystem.h"
#include "inc/gc/Object.h"

GC_Object* RTTypes::CreateFromFile(World &world, ObjectType type)
{
	FromFileMap::const_iterator it = _ffm.find(type);
	if( _ffm.end() == it )
		return NULL;
	return &it->second(world);
}

GC_Actor& RTTypes::CreateActor(World &world, ObjectType type, float x, float y)
{
	assert(IsRegistered(type));
    assert(!GetTypeInfo(type).service);
	return GetTypeInfo(type).CreateActor(world, x, y);
}

GC_Service& RTTypes::CreateService(World &world, ObjectType type)
{
    assert(IsRegistered(type));
    assert(GetTypeInfo(type).service);
    return GetTypeInfo(type).CreateService(world);
}
