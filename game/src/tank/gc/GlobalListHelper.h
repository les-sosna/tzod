#pragma once

#include "core/PtrList.h"

class GC_Object;


enum GlobalListID
{
	LIST_objects,
	LIST_services,
	LIST_respawns,
	LIST_players,
	LIST_sounds,
	LIST_indicators,
	LIST_vehicles,
	LIST_pickups,
	LIST_lights,
	LIST_cameras,
	//------------------
	GLOBAL_LIST_COUNT
};


#define DECLARE_MEMBER_OF(list)                         \
    private:                                            \
        PtrList<GC_Object>::id_type _pos##list;         \
    public:                                             \
        virtual void Register(World &world);            \
        virtual void Unregister(World &world)

#define IMPLEMENT_MEMBER_OF(cls, list)                  \
    void cls::Register(World &world)                    \
    {                                                   \
        base::Register(world);                          \
        _pos##list = world.GetList(list).insert(this);  \
    }                                                   \
    void cls::Unregister(World &world)                  \
    {                                                   \
        world.GetList(list).erase(_pos##list);          \
        base::Unregister(world);                        \
    }

