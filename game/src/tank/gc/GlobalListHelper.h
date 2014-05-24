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
        PtrList<GC_Object>::iterator _pos##list;        \
    public:                                             \
        virtual void Register(World &world);            \
        virtual void Unregister(World &world)

#define IMPLEMENT_MEMBER_OF(cls, list)                  \
    void cls::Register(World &world)                    \
    {                                                   \
        base::Register(world);                          \
        world.GetList(list).push_back(this);            \
        _pos##list = world.GetList(list).rbegin();      \
    }                                                   \
    void cls::Unregister(World &world)                  \
    {                                                   \
        world.GetList(list).safe_erase(_pos##list);     \
        base::Unregister(world);                        \
    }

