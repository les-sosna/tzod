#include "script.h"
#include "inc/script/ScriptHarness.h"
#include <gc/Pickup.h>
#include <gc/Player.h>
#include <gc/Trigger.h>
#include <gc/Vehicle.h>
#include <gc/World.h>
#include <gc/SaveFile.h>
#include "gclua/lObjUtil.h"

#include <fs/FileSystem.h>
extern "C"
{
#include <lua.h>
#include <lauxlib.h>
}
#include <pluto.h>

#include <sstream>


ScriptHarness::ScriptHarness(World &world, ScriptMessageSink &messageSink)
	: _world(world)
    , _messageSink(messageSink)
	, _L(script_open(world, messageSink))
	, _sPickup(_world, _L)
	, _sPlayer(_world, _L)
	, _sRigidBodyStatic(_world, _L)
	, _sTrigger(_world, _L)
{
	_world.eWorld.AddListener(*this);
}

ScriptHarness::~ScriptHarness()
{
	_world.eWorld.RemoveListener(*this);
	lua_close(_L);
}

void ScriptHarness::Step(float dt)
{
	RunCmdQueue(_L, dt, _messageSink);
}

void ScriptHarness::Serialize(SaveFile &f)
{
	struct WriteHelper
	{
		static int w(lua_State *L, const void* p, size_t sz, void* ud)
		{
			try
			{
				reinterpret_cast<SaveFile*>(ud)->GetStream().Write(p, sz);
			}
			catch( const std::exception &e )
			{
				return luaL_error(L, "[file write] %s", e.what());
			}
			return 0;
		}
		static int write_user(lua_State *L)
		{
			void *ud = lua_touserdata(L, 1);
			lua_settop(L, 0);
			lua_newtable(L);             // permanent objects
			lua_getfield(L, LUA_REGISTRYINDEX, "restore_ptr");
			lua_pushstring(L, "any_id_12345");
			lua_settable(L, -3);
			lua_getglobal(L, "user");    // object to persist
			pluto_persist(L, &w, ud);
			return 0;
		}
		static int write_queue(lua_State *L)
		{
			void *ud = lua_touserdata(L, 1);
			lua_settop(L, 0);
			lua_newtable(L);             // permanent objects
			lua_getglobal(L, "pushcmd");
			assert(LUA_TFUNCTION == lua_type(L, -1));
			lua_getupvalue(L, -1, 1);    // object to persist
			lua_remove(L, -2);
			pluto_persist(L, &w, ud);
			return 0;
		}
	};
	lua_newuserdata(_L, 0); // placeholder for restore_ptr
	lua_setfield(_L, LUA_REGISTRYINDEX, "restore_ptr");
	if( lua_cpcall(_L, &WriteHelper::write_user, &f) )
	{
		std::string err = "[pluto write user] ";
		err += lua_tostring(_L, -1);
		lua_pop(_L, 1);
		throw std::runtime_error(err);
	}
	if( lua_cpcall(_L, &WriteHelper::write_queue, &f) )
	{
		std::string err = "[pluto write queue] ";
		err += lua_tostring(_L, -1);
		lua_pop(_L, 1);
		throw std::runtime_error(err);
	}
	lua_setfield(_L, LUA_REGISTRYINDEX, "restore_ptr");
}

void ScriptHarness::Deserialize(SaveFile &f)
{
	struct ReadHelper
	{
		static const char* r(lua_State *L, void* data, size_t *sz)
		{
			static char buf[1];
			try
			{
				*sz = reinterpret_cast<FS::Stream*>(data)->Read(buf, 1, sizeof(buf));
			}
			catch( const std::exception &e )
			{
				*sz = 0;
				luaL_error(L, "deserialize error - %s", e.what());
			}
			return buf;
		}
		static int read_user(lua_State *L)
		{
			void *ud = lua_touserdata(L, 1);
			lua_settop(L, 0);
			lua_newtable(L);             // permanent objects
			lua_pushstring(L, "any_id_12345");
			lua_getfield(L, LUA_REGISTRYINDEX, "restore_ptr");
			lua_settable(L, -3);
			pluto_unpersist(L, &r, ud);
			lua_setglobal(L, "user");    // unpersisted object
			return 0;
		}
		static int read_queue(lua_State *L)
		{
			void *ud = lua_touserdata(L, 1);
			lua_settop(L, 0);
			lua_newtable(L);             // permanent objects
			pluto_unpersist(L, &r, ud);
			lua_getglobal(L, "pushcmd");
			assert(LUA_TFUNCTION == lua_type(L, -1));
			lua_pushvalue(L, -2);
			lua_setupvalue(L, -2, 1);    // unpersisted object
			return 0;
		}
		static int restore_ptr(lua_State *L)
		{
			assert(1 == lua_gettop(L));
			size_t id = (size_t) lua_touserdata(L, 1);
			SaveFile *fptr = (SaveFile *) lua_touserdata(L, lua_upvalueindex(1));
			assert(fptr);
			GC_Object *obj;
			try
			{
				obj = id ? fptr->RestorePointer(id) : nullptr;
			}
			catch( const std::exception &e )
			{
				return luaL_error(L, "%s", e.what());
			}
			luaT_pushobject(L, obj);
			return 1;
		}
	};
	lua_pushlightuserdata(_L, &f);
	lua_pushcclosure(_L, &ReadHelper::restore_ptr, 1);
	lua_setfield(_L, LUA_REGISTRYINDEX, "restore_ptr");
	if( lua_cpcall(_L, &ReadHelper::read_user, &f.GetStream()) )
	{
		std::string err = "[pluto read user] ";
		err += lua_tostring(_L, -1);
		lua_pop(_L, 1);
		throw std::runtime_error(err);
	}
	if( lua_cpcall(_L, &ReadHelper::read_queue, &f.GetStream()) )
	{
		std::string err = "[pluto read queue] ";
		err += lua_tostring(_L, -1);
		lua_pop(_L, 1);
		throw std::runtime_error(err);
	}
}

void ScriptHarness::OnGameStarted()
{
	if( !_world._infoOnInit.empty() )
	{
		script_exec(_L, _world._infoOnInit, "on_init");
	}
}
