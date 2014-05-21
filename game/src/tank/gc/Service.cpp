// Service.cpp

#include "Service.h"
#include "World.h"

// custom IMPLEMENT_MEMBER_OF for service
void GC_Service::Register(World &world)
{
    base::Register(world);
    _posLIST_services = world.GetList(LIST_services).insert(this);
	if( world._serviceListener )
		world._serviceListener->OnCreate(this);
}

void GC_Service::Unregister(World &world)
{
	if( world._serviceListener )
		world._serviceListener->OnKill(this);
    world.GetList(LIST_services).erase(_posLIST_services);
    base::Unregister(world);
}

// end of file
