#include "inc/luaetc/LuaDeleter.h"
extern "C"
{
#include <lua.h>
}

void LuaStateDeleter::operator()(lua_State *L)
{
	lua_close(L);
}
