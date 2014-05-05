#pragma once

#include "GlobalListHelper.h"
#include "globals.h"
#include "../World.h"

template<GlobalListID listId>
MemberOfGlobalList<listId>::MemberOfGlobalList(GC_Object *obj)
{
    g_level->GetList(listId).push_back(obj);
    _pos = g_level->GetList(listId).rbegin();
}

template<GlobalListID listId>
MemberOfGlobalList<listId>::~MemberOfGlobalList()
{
    g_level->GetList(listId).safe_erase(_pos);
}

