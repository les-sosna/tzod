#include "SaveFile.h"
#include "ScriptHarness.h"
#include "script.h"
#include "core/Debug.h"
#include "gc/Pickup.h"
#include "gc/Player.h"
#include "gc/Trigger.h"
#include "gc/Vehicle.h"
#include "gc/World.h"
#include "gclua/lObjUtil.h"

#include <fs/FileSystem.h>
extern "C"
{
#include <lua.h>
#include <lauxlib.h>
}
#include <pluto.h>

#include <sstream>


ScriptHarness::ScriptHarness(World &world, ScriptEnvironment &se)
	: _world(world)
	, _L(script_open(se))
{
	if (!_L) {
		throw std::bad_alloc();
	}
	
	_world.eGC_Trigger.AddListener(*this);
	_world.eGC_RigidBodyStatic.AddListener(*this);
	_world.eGC_Player.AddListener(*this);
	_world.eGC_Pickup.AddListener(*this);
}

ScriptHarness::~ScriptHarness()
{
	_world.eGC_Pickup.RemoveListener(*this);
	_world.eGC_Player.RemoveListener(*this);
	_world.eGC_RigidBodyStatic.RemoveListener(*this);
	_world.eGC_Trigger.RemoveListener(*this);
	
	script_close(_L);
}

void ScriptHarness::Step(float dt)
{
	RunCmdQueue(_L, dt);
}

void ScriptHarness::Serialize(std::shared_ptr<FS::Stream> stream, SaveFile &f)
{
	struct WriteHelper
	{
		static int w(lua_State *L, const void* p, size_t sz, void* ud)
		{
			try
			{
				reinterpret_cast<SaveFile*>(ud)->GetStream()->Write(p, sz);
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

void ScriptHarness::Deserialize(std::shared_ptr<FS::Stream> stream, SaveFile &f)
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
				obj = id ? fptr->RestorePointer(id) : NULL;
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
	if( lua_cpcall(_L, &ReadHelper::read_user, stream.get()) )
	{
		std::string err = "[pluto read user] ";
		err += lua_tostring(_L, -1);
		lua_pop(_L, 1);
		throw std::runtime_error(err);
	}
	if( lua_cpcall(_L, &ReadHelper::read_queue, stream.get()) )
	{
		std::string err = "[pluto read queue] ";
		err += lua_tostring(_L, -1);
		lua_pop(_L, 1);
		throw std::runtime_error(err);
	}
}


void ScriptHarness::OnEnter(GC_Trigger &obj, GC_Vehicle &vehicle)
{
	std::stringstream buf;
	buf << "return function(self,who)";
	buf << obj.GetOnEnter();
	buf << "\nend";
	
	if( luaL_loadstring(_L, buf.str().c_str()) )
	{
		GetConsole().Printf(1, "syntax error %s", lua_tostring(_L, -1));
		lua_pop(_L, 1); // pop the error message from the stack
	}
	else
	{
		if( lua_pcall(_L, 0, 1, 0) )
		{
			GetConsole().WriteLine(1, lua_tostring(_L, -1));
			lua_pop(_L, 1); // pop the error message from the stack
		}
		else
		{
			luaT_pushobject(_L, &obj);
			luaT_pushobject(_L, &vehicle);
			if( lua_pcall(_L, 2, 0, 0) )
			{
				GetConsole().WriteLine(1, lua_tostring(_L, -1));
				lua_pop(_L, 1); // pop the error message from the stack
			}
		}
	}
}

void ScriptHarness::OnLeave(GC_Trigger &obj)
{
	script_exec(_L, obj.GetOnLeave().c_str());
}


void ScriptHarness::OnDestroy(GC_RigidBodyStatic &obj)
{
	if( !obj.GetOnDestroy().empty() )
	{
		script_exec(_L, obj.GetOnDestroy().c_str());
	}
}

void ScriptHarness::OnDamage(GC_RigidBodyStatic &obj, GC_Actor *from)
{
	if( !obj.GetOnDamage().empty() )
	{
		if( from )
		{
			std::stringstream buf;
			buf << "return function(who)";
			buf << obj.GetOnDamage();
			buf << "\nend";
			
			if( luaL_loadstring(_L, buf.str().c_str()) )
			{
				GetConsole().Printf(1, "OnDamage: %s", lua_tostring(_L, -1));
				lua_pop(_L, 1); // pop the error message from the stack
			}
			else
			{
				if( lua_pcall(_L, 0, 1, 0) )
				{
					GetConsole().WriteLine(1, lua_tostring(_L, -1));
					lua_pop(_L, 1); // pop the error message from the stack
				}
				else
				{
					luaT_pushobject(_L, from);
					if( lua_pcall(_L, 1, 0, 0) )
					{
						GetConsole().WriteLine(1, lua_tostring(_L, -1));
						lua_pop(_L, 1); // pop the error message from the stack
					}
				}
			}
		}
		else
		{
			script_exec(_L, obj.GetOnDamage().c_str());
		}
	}
}


void ScriptHarness::OnPickup(GC_Pickup &obj, GC_Actor &actor)
{
	if( !obj.GetOnPickup().empty() )
	{
		std::stringstream buf;
		buf << "return function(who)";
		buf << obj.GetOnPickup();
		buf << "\nend";
		
		if( luaL_loadstring(_L, buf.str().c_str()) )
		{
			GetConsole().Printf(1, "OnPickup: %s", lua_tostring(_L, -1));
			lua_pop(_L, 1); // pop the error message from the stack
		}
		else
		{
			if( lua_pcall(_L, 0, 1, 0) )
			{
				GetConsole().WriteLine(1, lua_tostring(_L, -1));
				lua_pop(_L, 1); // pop the error message from the stack
			}
			else
			{
				luaT_pushobject(_L, &actor);
				if( lua_pcall(_L, 1, 0, 0) )
				{
					GetConsole().WriteLine(1, lua_tostring(_L, -1));
					lua_pop(_L, 1); // pop the error message from the stack
				}
			}
		}
	}
}


void ScriptHarness::OnRespawn(GC_Player &obj, GC_Vehicle &vehicle)
{
	if( !obj.GetOnRespawn().empty() )
	{
		script_exec(_L, obj.GetOnRespawn().c_str());
	}
}

void ScriptHarness::OnDie(GC_Player &obj)
{
	if( !obj.GetOnDie().empty() )
	{
		script_exec(_L, obj.GetOnDie().c_str());
	}
}

