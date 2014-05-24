// Service.cpp

#include "Service.h"
#include "World.h"

// custom IMPLEMENT_MEMBER_OF for service
void GC_Service::Register(World &world)
{
    base::Register(world);
    world.GetList(LIST_services).push_back(this);
    _posLIST_services = world.GetList(LIST_services).rbegin();
	if( world._serviceListener )
		world._serviceListener->OnCreate(this);
}

void GC_Service::Unregister(World &world)
{
	if( world._serviceListener )
		world._serviceListener->OnKill(this);
    world.GetList(LIST_services).safe_erase(_posLIST_services);
    base::Unregister(world);
}

// end of file
