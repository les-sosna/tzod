// Service.cpp

#include "Service.h"
#include "World.h"

// custom IMPLEMENT_1LIST_MEMBER for service
PtrList<GC_Object>::id_type GC_Service::Register(World &world)
{
    PtrList<GC_Object>::id_type pos = base::Register(world);
    world.GetList(LIST_services).insert(this, pos);
	if( world._serviceListener )
		world._serviceListener->OnCreate(this);
    return pos;
}

void GC_Service::Unregister(World &world, PtrList<GC_Object>::id_type pos)
{
	if( world._serviceListener )
		world._serviceListener->OnKill(this);
    world.GetList(LIST_services).erase(pos);
    base::Unregister(world, pos);
}

// end of file
