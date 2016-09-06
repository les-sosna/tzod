#include "script.h"
#include "inc/script/detail/sPickup.h"
#include <gc/Pickup.h>
#include <gc/Vehicle.h>
#include <gc/World.h>
#include "gclua/lObjUtil.h"
extern "C"
{
#include <lua.h>
#include <lauxlib.h>
}
#include <sstream>
#include <stdexcept>

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

void sPickup::OnAttach(GC_Pickup &obj, GC_Vehicle &vehicle, bool asInitial)
{
	if( !obj.GetOnPickup().empty() )
	{
		std::stringstream buf;
		buf << "return function(who)";
		buf << obj.GetOnPickup();
		buf << "\nend";

		if( luaL_loadstring(_L, buf.str().c_str()) )
		{
            std::runtime_error error(lua_tostring(_L, -1));
			lua_pop(_L, 1); // pop the error message from the stack
            throw error;
		}
		else
		{
			if( lua_pcall(_L, 0, 1, 0) )
			{
                std::runtime_error error(lua_tostring(_L, -1));
				lua_pop(_L, 1); // pop the error message from the stack
                throw error;
			}
			else
			{
				luaT_pushobject(_L, &vehicle);
				if( lua_pcall(_L, 1, 0, 0) )
				{
                    std::runtime_error error(lua_tostring(_L, -1));
					lua_pop(_L, 1); // pop the error message from the stack
                    throw error;
				}
			}
		}
	}
}
