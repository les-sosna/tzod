// script.cpp

#include "script.h"

#include "core/Debug.h"

#include "gc/vehicle.h"

#include "gclua/lGame.h"
#include "gclua/lObject.h"
#include "gclua/lWorld.h"

#include <fs/FileSystem.h>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}


void ClearCommandQueue(lua_State *L)
{
	lua_getglobal(L, "pushcmd");
	 assert(LUA_TFUNCTION == lua_type(L, -1));
	 lua_newtable(L);
	  lua_setupvalue(L, -2, 1); // pops result of lua_newtable
	 lua_pop(L, 1);  // pop result of lua_getglobal
}

void RunCmdQueue(lua_State *L, float dt)
{
	lua_getglobal(L, "pushcmd");
	assert(LUA_TFUNCTION == lua_type(L, -1));
	lua_getupvalue(L, -1, 1);
	int queueidx = lua_gettop(L);
    
	for( lua_pushnil(L); lua_next(L, queueidx); lua_pop(L, 1) )
	{
		// -2 -> key; -1 -> value(table)
        
		lua_rawgeti(L, -1, 2);
		lua_Number time = lua_tonumber(L, -1) - dt;
		lua_pop(L, 1);
        
		if( time <= 0 )
		{
			// call function and remove it from queue
			lua_rawgeti(L, -1, 1);
			if( lua_pcall(L, 0, 0, 0) )
			{
				GetConsole().WriteLine(1, lua_tostring(L, -1));
				lua_pop(L, 1); // pop the error message
			}
			lua_pushvalue(L, -2); // push copy of the key
			lua_pushnil(L);
			lua_settable(L, queueidx);
		}
		else
		{
			// update time value
			lua_pushnumber(L, time);
			lua_rawseti(L, -2, 2);
		}
	}
    
	assert(lua_gettop(L) == queueidx);
	lua_pop(L, 2); // pop results of lua_getglobal and lua_getupvalue
}

static int luaT_print(lua_State *L)
{
	std::stringstream buf;
	int n = lua_gettop(L);     // get number of arguments
	lua_getglobal(L, "tostring");
	for( int i = 1; i <= n; ++i )
	{
		lua_pushvalue(L, -1);  // function to be called
		lua_pushvalue(L, i);   // value to print (1-st arg)
		lua_call(L, 1, 1);
		const char *s = lua_tostring(L, -1);  // get result string
		if( NULL == s )
		{
			return luaL_error(L, LUA_QL("tostring") " must return a string to " LUA_QL("print"));
		}
		if( i > 1 ) buf << " "; // delimiter
		buf << s;
		lua_pop(L, 1);         // pop result
	}
	GetConsole().WriteLine(0, buf.str());
	return 0;
}

int luaT_pushcmd(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TFUNCTION);
	if( 1 == lua_gettop(L) )
		lua_pushnumber(L, 0);
	luaL_checktype(L, 2, LUA_TNUMBER);
	lua_settop(L, 2);

	lua_createtable(L, 2, 0); // create a new table at index 3
	lua_pushvalue(L, 1);      // push copy of the function
	lua_rawseti(L, 3, 1);
	lua_pushvalue(L, 2);      // push copy of the delay
	lua_rawseti(L, 3, 2);

	lua_rawseti(L, lua_upvalueindex(1), lua_objlen(L, lua_upvalueindex(1)) + 1);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// api

lua_State* script_open(ScriptEnvironment &se)
{
	lua_State *L = luaL_newstate();
    if (!L)
        throw std::bad_alloc();

	//
	// open libs
	//

	static const luaL_Reg lualibs[] = {
		// standard Lua libs
		{"", luaopen_base},
		{LUA_LOADLIBNAME, luaopen_package},
		{LUA_TABLIBNAME, luaopen_table},
//		{LUA_IOLIBNAME, luaopen_io},
//		{LUA_OSLIBNAME, luaopen_os},
		{LUA_STRLIBNAME, luaopen_string},
		{LUA_MATHLIBNAME, luaopen_math},
#ifdef _DEBUG
		{LUA_DBLIBNAME, luaopen_debug},
#endif
		
		// game libs
		{"game", luaopen_game},
		{"object", luaopen_object},
		{"world", luaopen_world},
		
		{NULL, NULL}
	};

	for( const luaL_Reg *lib = lualibs; lib->func; ++lib )
	{
		lua_pushcfunction(L, lib->func);
		lua_pushstring(L, lib->name);
		lua_call(L, 1, 0);
	}
    
	// set script environment
    lua_pushlightuserdata(L, &se);
    lua_setfield(L, LUA_REGISTRYINDEX, "ENVIRONMENT");

	// override default print function so it will print to console
	lua_register(L, "print", luaT_print);

	// init the command queue
	lua_newtable(L);
	lua_pushcclosure(L, luaT_pushcmd, 1);
	lua_setglobal(L, "pushcmd");


	//
	// create global 'gc' table and fill it with editor classes
	//

	lua_newtable(L);
	for( int i = 0; i < RTTypes::Inst().GetTypeCount(); ++i )
	{
		lua_newtable(L);
		lua_setfield(L, -2, RTTypes::Inst().GetTypeInfoByIndex(i).name);
	}
	lua_setglobal(L, "gc"); // set global and pop one element from stack


	lua_newtable(L);
	lua_setglobal(L, "classes");

	lua_newtable(L);
	lua_setglobal(L, "user");
	
	return L;
}

void script_close(lua_State *L)
{
	assert(L);
	lua_close(L);
}

bool script_exec(lua_State *L, const char *string)
{
	if( luaL_loadstring(L, string) )
	{
		GetConsole().Printf(1, "syntax error %s", lua_tostring(L, -1));
		lua_pop(L, 1); // pop the error message from the stack
		return false;
	}
	if( lua_pcall(L, 0, 0, 0) )
	{
		GetConsole().WriteLine(1, lua_tostring(L, -1));
		lua_pop(L, 1); // pop the error message from the stack
		return false;
	}
	return true;
}

bool script_exec_file(lua_State *L, FS::FileSystem &fs, const char *filename)
{
	try
	{
		std::shared_ptr<FS::MemMap> f = fs.Open(filename)->QueryMap();
		if( luaL_loadbuffer(L, f->GetData(), f->GetSize(), filename) )
		{
			std::string msg(lua_tostring(L, -1));
			lua_pop(L, 1); // pop error message
			throw std::runtime_error(msg);
		}
		if( lua_pcall(L, 0, 0, 0) )
		{
			std::string err = lua_tostring(L, -1);
			lua_pop(L, 1); // pop the error message from the stack
			throw std::runtime_error(std::string("runtime error: ") + err);
		}
	}
	catch( const std::runtime_error &e )
	{
		GetConsole().WriteLine(1, e.what());
		return false;
	}
	return true;
}

// end of file
