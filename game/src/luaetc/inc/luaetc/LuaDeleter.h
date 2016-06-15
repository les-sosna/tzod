#pragma once

struct lua_State;

struct LuaStateDeleter
{
	void operator()(lua_State *L);
};
