#pragma once

struct lua_State;
struct ENVIRONMENT
{
	lua_State *L;
};
extern ENVIRONMENT g_env;
