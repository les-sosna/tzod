#include "sTrigger.h"
#include "script.h"
#include "core/Debug.h"
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

sTrigger::sTrigger(World &world, lua_State *L)
	: _world(world)
	, _L(L)
{
	_world.eGC_Trigger.AddListener(*this);
}
	
sTrigger::~sTrigger()
{
	_world.eGC_Trigger.RemoveListener(*this);
}

void sTrigger::OnEnter(GC_Trigger &obj, GC_Vehicle &vehicle)
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

void sTrigger::OnLeave(GC_Trigger &obj)
{
	script_exec(_L, obj.GetOnLeave().c_str());
}

