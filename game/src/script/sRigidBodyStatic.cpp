#include "inc/script/detail/sRigidBodyStatic.h"
#include "script.h"
#include <gc/Player.h>
#include <gc/RigidBody.h>
#include <gc/Trigger.h>
#include <gc/World.h>
#include "gclua/lObjUtil.h"
extern "C"
{
#include <lua.h>
#include <lauxlib.h>
}
#include <sstream>
#include <stdexcept>

sRigidBodyStatic::sRigidBodyStatic(World &world, lua_State *L)
	: _world(world)
	, _L(L)
{
	_world.eGC_RigidBodyStatic.AddListener(*this);
}

sRigidBodyStatic::~sRigidBodyStatic()
{
	_world.eGC_RigidBodyStatic.RemoveListener(*this);
}

void sRigidBodyStatic::OnDestroy(GC_RigidBodyStatic &obj, const DamageDesc &dd)
{
	if( !obj.GetOnDestroy().empty() )
	{
		script_exec(_L, obj.GetOnDestroy().c_str());
	}
}

void sRigidBodyStatic::OnDamage(GC_RigidBodyStatic &obj, const DamageDesc &dd)
{
	if( !obj.GetOnDamage().empty() )
	{
		if( dd.from )
		{
			std::stringstream buf;
			buf << "return function(who)";
			buf << obj.GetOnDamage();
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
					luaT_pushobject(_L, dd.from);
					if( lua_pcall(_L, 1, 0, 0) )
					{
                        std::runtime_error error(lua_tostring(_L, -1));
						lua_pop(_L, 1); // pop the error message from the stack
                        throw error;
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

