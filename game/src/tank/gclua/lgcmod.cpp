#include "lgcmod.h"
extern "C"
{
#include <lua.h>
#include <lauxlib.h>
}

ScriptEnvironment& GetScriptEnvironment(lua_State *L)
{
	lua_getfield(L, LUA_REGISTRYINDEX, "ENVIRONMENT");
	auto *se = (ScriptEnvironment *) lua_touserdata(L, -1);
	lua_pop(L, 1);
	if (!se) {
		luaL_error(L, "environment is not properly initialized");
	}
	return *se;
}
