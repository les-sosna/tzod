#include "inc/gclua/lObjUtil.h"
#include "inc/gclua/lWorld.h"
#include <gc/MovingObject.h>
#include <gc/Service.h>
#include <gc/TypeSystem.h>
#include <gc/World.h>
extern "C"
{
#include <lua.h>
#include <lauxlib.h>
}

// actor("type name", x, y [, params])
int world_actor(lua_State *L)
{
	int n = lua_gettop(L);
	if( 3 != n && 4 != n )
	{
		return luaL_error(L, "3 or 4 arguments expected; got %d", n);
	}

	const char *name = luaL_checkstring(L, 1);
	float x = (float) luaL_checknumber(L, 2);
	float y = (float) luaL_checknumber(L, 3);

	ObjectType type = RTTypes::Inst().GetTypeByName(name);
	if( INVALID_OBJECT_TYPE == type )
	{
		return luaL_error(L, "unknown type '%s'", name);
	}

	if( RTTypes::Inst().GetTypeInfo(type).service )
	{
		return luaL_error(L, "type '%s' is a service", name);
	}

    World &world = luaT_getworld(L);
    GC_MovingObject &obj = RTTypes::Inst().CreateObject(world, type, vec2d{x, y});

	if( 4 == n )
	{
		luaL_checktype(L, 4, LUA_TTABLE);
		std::shared_ptr<PropertySet> properties = obj.GetProperties(world);

		for( lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1) )
		{
			// now 'key' is at index -2 and 'value' at index -1
			luaT_setproperty(L, *properties);
		}

		properties->Exchange(world, true);
	}

	luaT_pushobject(L, &obj);
	return 1;
}

// service("type name" [, params])
int world_service(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n && 2 != n )
	{
		return luaL_error(L, "1 or 2 arguments expected; got %d", n);
	}

	const char *name = luaL_checkstring(L, 1);

	ObjectType type = RTTypes::Inst().GetTypeByName(name);
	if( INVALID_OBJECT_TYPE == type )
	{
		return luaL_error(L, "unknown type '%s'", name);
	}

	if( !RTTypes::Inst().GetTypeInfo(type).service )
	{
		return luaL_error(L, "type '%s' is not a service", name);
	}

    World &world = luaT_getworld(L);
	GC_Service &obj = RTTypes::Inst().CreateService(world, type);


	if( 2 == n )
	{
		luaL_checktype(L, 2, LUA_TTABLE);

		std::shared_ptr<PropertySet> properties = obj.GetProperties(world);

		for( lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1) )
		{
			// now 'key' is at index -2 and 'value' at index -1
			luaT_setproperty(L, *properties);
		}

		properties->Exchange(world, true);
	}

	luaT_pushobject(L, &obj);
	return 1;
}

// object("object name")
int world_object(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
	{
		return luaL_error(L, "1 argument expected; got %d", n);
	}

	const char *name = luaL_checkstring(L, 1);

    World &world = luaT_getworld(L);
	GC_Object *obj = world.FindObject(name);
	if( !obj )
	{
		luaL_error(L, "object with name '%s' does not exist", name);
	}

	luaT_pushobject(L, obj);
	return 1;
}

// exists("object name")
int world_exists(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
	{
		return luaL_error(L, "1 argument expected; got %d", n);
	}

	const char *name = luaL_checkstring(L, 1);
	World &world = luaT_getworld(L);
	lua_pushboolean(L, nullptr != world.FindObject(name));
	return 1;
}

static const luaL_Reg worldlib[] = {
	{"actor", world_actor},
	{"service", world_service},
	{"object", world_object},
	{"exists", world_exists},
	{nullptr, nullptr}
};

int luaopen_world(lua_State *L)
{
	luaL_register(L, "world", worldlib);
	return 1;
}
