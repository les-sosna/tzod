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

template<GlobalListID listId>
class MemberOfGlobalList
{
    PtrList<GC_Object>::iterator  _pos;
public:
    MemberOfGlobalList(GC_Object *obj);
    ~MemberOfGlobalList();
};

