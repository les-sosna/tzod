#include "LuaConsole.h"
#include <config/ConfigBase.h>
#include <ui/ConsoleBuffer.h>
#include <stdexcept>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

static int print(lua_State *L)
{
	std::stringstream buf;
	int n = lua_gettop(L);     // get number of arguments
	lua_getglobal(L, "tostring");
	for (int i = 1; i <= n; ++i)
	{
		lua_pushvalue(L, -1);  // function to be called
		lua_pushvalue(L, i);   // value to print (1-st arg)
		lua_call(L, 1, 1);
		const char *s = lua_tostring(L, -1);  // get result string
		if (nullptr == s)
		{
			return luaL_error(L, LUA_QL("tostring") " must return a string to " LUA_QL("print"));
		}
		if (i > 1) buf << " "; // delimiter
		buf << s;
		lua_pop(L, 1);         // pop call result
	}
	auto logger = reinterpret_cast<UI::ConsoleBuffer*>(lua_touserdata(L, lua_upvalueindex(1)));
	logger->Format(0) << buf.str();
	return 0;
}

static int pinit(lua_State *L)
{
	assert(lua_gettop(L) == 1); // logger

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
		{ nullptr, nullptr }
	};

	for (const luaL_Reg *lib = lualibs; lib->func; ++lib)
	{
		lua_pushcfunction(L, lib->func);
		lua_pushstring(L, lib->name);
		lua_call(L, 1, 0);
	}

	// override default print function so it will print to console
	assert(lua_gettop(L) == 1); // logger
	lua_pushcclosure(L, print, 1);
	lua_setglobal(L, "print");

	return 0;
}

static std::string_view tostringview(lua_State *L)
{
	size_t size = 0;
	const char *buf = lua_tolstring(L, -1, &size);
	return std::string_view(buf, size);
}


LuaConsole::LuaConsole(UI::ConsoleBuffer &logger, ConfVarTable &configRoot)
	: _logger(logger)
	, _L(luaL_newstate())
{
	if (lua_cpcall(_L.get(), pinit, &logger))
	{
		const char *what = lua_tostring(_L.get(), -1);
		std::runtime_error error(what ? what : "Unknown error");
		lua_pop(_L.get(), 1);
		throw error;
	}

	configRoot.InitConfigLuaBinding(_L.get(), "conf");
}

void LuaConsole::Exec(std::string_view cmd)
{
	// first try to load as is, then try wrapping with print()
	bool success = false;
	std::string errmsg;
	if (luaL_loadbuffer(_L.get(), cmd.data(), cmd.size(), "command"))
	{
		errmsg = tostringview(_L.get());
		lua_pop(_L.get(), 1);

		auto expr = std::string("print(").append(cmd).append(")");

		if (luaL_loadbuffer(_L.get(), expr.data(), expr.size(), "expression"))
		{
			lua_pop(_L.get(), 1); // ignore this error and report the one from above
		}
		else
		{
			success = true;
		}
	}
	else
	{
		success = true;
	}

	if (success && lua_pcall(_L.get(), 0, 0, 0))
	{
		success = false;
		errmsg = tostringview(_L.get());
		lua_pop(_L.get(), 1);
	}

	if (!success)
	{
		_logger.Format(1) << (errmsg.empty() ? "Unknown error" : errmsg);
	}
}

bool LuaConsole::CompleteCommand(std::string_view cmd, int &pos, std::string &result)
{
	assert(pos >= 0);
	lua_getglobal(_L.get(), "autocomplete"); // FIXME: can potentially throw
	if (lua_isnil(_L.get(), -1))
	{
		lua_pop(_L.get(), 1);
		_logger.WriteLine(1, "There was no autocomplete module loaded");
		return false;
	}
	lua_pushlstring(_L.get(), cmd.substr(0, pos).data(), pos);
	if (lua_pcall(_L.get(), 1, 1, 0))
	{
		_logger.WriteLine(1, lua_tostring(_L.get(), -1));
		lua_pop(_L.get(), 1); // pop error message
	}
	else
	{
		const char *str = lua_tostring(_L.get(), -1);
		std::string insert = str ? str : "";

		result = std::string(cmd.substr(0, pos)).append(insert).append(cmd.substr(pos));
		pos += static_cast<int>(insert.length());

		if (!result.empty() && result[0] != '/')
		{
			result = std::string("/") + result;
			++pos;
		}
	}
	lua_pop(_L.get(), 1); // pop result or error message
	return true;
}
