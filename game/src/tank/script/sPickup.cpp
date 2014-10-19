#include "sPickup.h"
#include "script.h"
#include "core/Debug.h"
#include "gc/Pickup.h"
#include "gc/Vehicle.h"
#include "gc/World.h"
#include "gclua/lObjUtil.h"
extern "C"
{
#include <lua.h>
#include <lauxlib.h>
}
#include <sstream>

sPickup::sPickup(World &world, lua_State *L)
	: _world(world)
	, _L(L)
{
	_world.eGC_Pickup.AddListener(*this);
}

sPickup::~sPickup()
{
	_world.eGC_Pickup.RemoveListener(*this);
}

void sPickup::OnAttach(GC_Pickup &obj, GC_Vehicle &vehicle)
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
				luaT_pushobject(_L, &vehicle);
				if( lua_pcall(_L, 1, 0, 0) )
				{
					GetConsole().WriteLine(1, lua_tostring(_L, -1));
					lua_pop(_L, 1); // pop the error message from the stack
				}
			}
		}
	}
}
