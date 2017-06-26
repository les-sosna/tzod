#include "inc/editor/detail/QuickActions.h"
#include <gc/Object.h>
#include <gc/TypeSystem.h>
#include "gclua/lObject.h"
#include <gclua/lObjUtil.h>
#include "gclua/lWorld.h"
#include <ui/ConsoleBuffer.h>
#include <stdexcept>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

static int init(lua_State *L)
{
	lua_setfield(L, LUA_REGISTRYINDEX, "WORLD");

	//
	// open libs
	//

	static const luaL_Reg lualibs[] = {
		// standard Lua libs
		{ "", luaopen_base },
		{ LUA_LOADLIBNAME, luaopen_package },
		{ LUA_TABLIBNAME, luaopen_table },
		{ LUA_STRLIBNAME, luaopen_string },
		{ LUA_MATHLIBNAME, luaopen_math },
#ifdef _DEBUG
		{ LUA_DBLIBNAME, luaopen_debug },
#endif

		// game libs
		{ "object", luaopen_object },
		{ "world", luaopen_world },

		{ nullptr, nullptr }
	};

	for (const luaL_Reg *lib = lualibs; lib->func; ++lib)
	{
		lua_pushcfunction(L, lib->func);
		lua_pushstring(L, lib->name);
		lua_call(L, 1, 0);
	}

	return 0;
}

static int doaction(lua_State *L)
{
	auto object = reinterpret_cast<GC_Object*>(lua_touserdata(L, 1));

	// qa = require "quick_actions"
	lua_getglobal(L, "require");
	lua_pushliteral(L, "data/scripts/quick_actions");
	lua_call(L, 1, 1);

	// action = qa[typename]
	auto &selTypeInfo = RTTypes::Inst().GetTypeInfo(object->GetType());
	lua_getfield(L, -1, selTypeInfo.name);

	// if action ~= null then action(object) end
	if (!lua_isnil(L, -1))
	{
		luaT_pushobject(L, object);
		lua_call(L, 1, 0);
	}

	return 0;
}

QuickActions::QuickActions(UI::ConsoleBuffer &logger, World &world)
	: _logger(logger)
	, _L(luaL_newstate())
{
	if (lua_cpcall(_L.get(), init, &world))
	{
		const char *what = lua_tostring(_L.get(), -1);
		std::runtime_error error(what ? what : "Unknown error");
		lua_pop(_L.get(), 1);
		throw error;
	}
}

void QuickActions::DoAction(GC_Object &object)
{
	if (lua_cpcall(_L.get(), doaction, &object))
	{
		const char *cwhat = lua_tostring(_L.get(), -1);
		std::string what = cwhat ? cwhat : "Unknown error";
		lua_pop(_L.get(), 1); // pop error message
		_logger.WriteLine(1, what);
	}
}
