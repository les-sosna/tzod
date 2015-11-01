#pragma once

#include "PtrList.h"

class GC_Object;


enum GlobalListID
{
	LIST_objects,
    LIST_timestep,
	LIST_services,
	LIST_respawns,
	LIST_players,
	LIST_indicators,
	LIST_vehicles,
	LIST_pickups,
	LIST_lights,
    LIST_gsprites,
	//------------------
	GLOBAL_LIST_COUNT
};


#define DECLARE_LIST_MEMBER(OVERRIDE)                                   \
protected:                                                              \
    virtual PtrList<GC_Object>::id_type Register(World &world) OVERRIDE;\
    virtual void Unregister(World &world, PtrList<GC_Object>::id_type pos) OVERRIDE; \
private:                                                                \
    friend class World;                                                 \
	friend class RTTypes

#define IMPLEMENT_1LIST_MEMBER(cls, list)                               \
    PtrList<GC_Object>::id_type cls::Register(World &world)             \
    {                                                                   \
        auto pos = base::Register(world);                               \
        world.GetList(list).insert(this, pos);                          \
        return pos;                                                     \
    }                                                                   \
    void cls::Unregister(World &world, PtrList<GC_Object>::id_type pos) \
    {                                                                   \
        world.GetList(list).erase(pos);                                 \
        base::Unregister(world, pos);                                   \
    }

#define IMPLEMENT_2LIST_MEMBER(cls, list1, list2)                       \
    PtrList<GC_Object>::id_type cls::Register(World &world)             \
    {                                                                   \
        auto pos = base::Register(world);                               \
        world.GetList(list1).insert(this, pos);                         \
        world.GetList(list2).insert(this, pos);                         \
        return pos;                                                     \
    }                                                                   \
    void cls::Unregister(World &world, PtrList<GC_Object>::id_type pos) \
    {                                                                   \
        world.GetList(list2).erase(pos);                                \
        world.GetList(list1).erase(pos);                                \
        base::Unregister(world, pos);                                   \
    }

