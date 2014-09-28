#include "ScriptHarness.h"
#include "script.h"
#include "core/Debug.h"
#include "gc/Pickup.h"
#include "gc/Player.h"
#include "gc/Trigger.h"
#include "gc/Vehicle.h"
#include "gc/World.h"
#include "gclua/lObjUtil.h"

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
}

#include <sstream>


ScriptHarness::ScriptHarness(World &world, lua_State *L)
	: _world(world)
	, _L(L)
{
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

