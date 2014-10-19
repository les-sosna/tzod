#include "lObject.h"
#include "lObjUtil.h"
#include "lgcmod.h"
#include "SaveFile.h"
#include "gc/ObjPtr.h"
#include "gc/Object.h"
#include "gc/Pickup.h"
#include "gc/RigidBody.h"
#include "gc/Vehicle.h"
#include "gc/Player.h"
#include "gc/Weapons.h"
#include "gc/World.h"

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
}



//
// Object methamethods
//

static int objfinalizer(lua_State *L)
{
	assert(1 == lua_gettop(L));
	((ObjPtr<GC_Object> *) lua_touserdata(L, 1))->~ObjPtr();
	return 0;
}

static int objpersist(lua_State *L)
{
	assert(3 == lua_gettop(L));
	ObjPtr<GC_Object> *ppObj = (ObjPtr<GC_Object> *) lua_touserdata(L, 1);
	auto f = (SaveFile *) lua_touserdata(L, 3);
	assert(f);

	if( luaL_loadstring(L,
		"return function(restore_ptr, id) return function() return restore_ptr(id) end end") )
	{
		return lua_error(L); // use error message on stack
	}
	lua_call(L, 0, 1);

	lua_getfield(L, LUA_REGISTRYINDEX, "restore_ptr");
	lua_pushlightuserdata(L, (void *) f->GetPointerId(*ppObj));
	lua_call(L, 2, 1);
	return 1;
}

static void pushprop(lua_State *L, ObjectProperty *p)
{
	assert(L && p);
	switch( p->GetType() )
	{
	case ObjectProperty::TYPE_INTEGER:
		lua_pushinteger(L, p->GetIntValue());
		break;
	case ObjectProperty::TYPE_FLOAT:
		lua_pushnumber(L, p->GetFloatValue());
		break;
	case ObjectProperty::TYPE_STRING:
	case ObjectProperty::TYPE_SKIN:
	case ObjectProperty::TYPE_TEXTURE:
		lua_pushstring(L, p->GetStringValue().c_str());
		break;
	case ObjectProperty::TYPE_MULTISTRING:
		lua_pushstring(L, p->GetListValue(p->GetCurrentIndex()).c_str());
		break;
	default:
		assert(false);
	}
}

// generic __next support for object properties
static int objnextprop(lua_State *L)
{
	lua_settop(L, 2);

	ObjPtr<GC_Object> *ppObj = (ObjPtr<GC_Object> *) luaL_checkudata(L, 1, "object");
	if( !*ppObj )
	{
		luaL_argerror(L, 1, "reference to a dead object");
	}

    World &world = GetScriptEnvironment(L).world;
	std::shared_ptr<PropertySet> properties = (*ppObj)->GetProperties(world);
	assert(properties);

	if( lua_isnil(L, 2) )
	{
		// begin iteration
		ObjectProperty *p = properties->GetProperty(0);
		lua_pushstring(L, p->GetName().c_str()); // key
		pushprop(L, p); // value
		return 2;
	}

	const char *key = luaL_checkstring(L, 2);

	bool found = false;
	int count = properties->GetCount();
	for( int i = 0; i < count; ++i )
	{
		ObjectProperty *p = properties->GetProperty(i);
		if( found )
		{
			// return next pair
			lua_pushstring(L, p->GetName().c_str()); // key
			pushprop(L, p); // value
			return 2;
		}
		else if( p->GetName() == key )
		{
			found = true;
		}
	}

	if( found )
	{
		// end of list
		lua_pushnil(L);
		return 1;
	}

	return luaL_error(L, "invalid key to 'next'");
}

static int pget(lua_State *L)
{
	int n = lua_gettop(L);
	if( 2 != n )
	{
		return luaL_error(L, "2 arguments expected; got %d", n);
	}
	
	GC_Object *obj = luaT_checkobject(L, 1);
	const char *prop = luaL_checkstring(L, 2);
	
	ScriptEnvironment &se = GetScriptEnvironment(L);
	std::shared_ptr<PropertySet> properties = obj->GetProperties(se.world);
	assert(properties);
	
	for( int i = 0; i < properties->GetCount(); ++i )
	{
		ObjectProperty *p = properties->GetProperty(i);
		if( p->GetName() == prop )
		{
			pushprop(L, p);
			return 1;
		}
	}
	
	return luaL_error(L, "object of type '%s' does not have property '%s'",
					  RTTypes::Inst().GetTypeName(obj->GetType()), prop);
}

static int pset(lua_State *L)
{
	int n = lua_gettop(L);
	if( 3 != n )
	{
		return luaL_error(L, "3 arguments expected; got %d", n);
	}
	
	GC_Object *obj = luaT_checkobject(L, 1);
	const char *prop = luaL_checkstring(L, 2);
	luaL_checkany(L, 3);  // prop value should be here
	
	ScriptEnvironment &se = GetScriptEnvironment(L);
	std::shared_ptr<PropertySet> properties = obj->GetProperties(se.world);
	
	// prop name at -2; prop value at -1
	if( !luaT_setproperty(L, *properties) )
	{
		return luaL_error(L, "object of type '%s' has no property '%s'",
						  RTTypes::Inst().GetTypeName(obj->GetType()), prop);
	}
	
	properties->Exchange(se.world, true);
	return 0;
}



//
// Object Lua API
//

template<class T>
static T* checkobject(lua_State *L, int n)
{
	T *result = dynamic_cast<T *>(luaT_checkobject(L, n));
	if( !result )
	{
		luaL_argerror(L, n, "incompatible object type");
	}
	return result;
}


// damage("victim", hp)
static int object_damage(lua_State *L)
{
	int n = lua_gettop(L);
	if( 2 != n )
	{
		return luaL_error(L, "2 arguments expected; got %d", n);
	}

	GC_RigidBodyStatic *rbs = checkobject<GC_RigidBodyStatic>(L, 1);
	float hp = (float) luaL_checknumber(L, 2);

    World &world = GetScriptEnvironment(L).world;
	rbs->TakeDamage(world, DamageDesc{hp, rbs->GetPos(), nullptr});

	return 0;
}

// kill("object name")
static int object_kill(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
	{
		return luaL_error(L, "1 argument expected; got %d", n);
	}

    World &world = GetScriptEnvironment(L).world;
	luaT_checkobject(L, 1)->Kill(world);
	return 0;
}

//setposition(name,x,y)
static int object_setposition(lua_State *L)
{
	int n = lua_gettop(L);
	if( 3 != n )
	{
		return luaL_error(L, "3 arguments expected; got %d", n);
	}

	GC_Actor *actor = checkobject<GC_Actor>(L, 1);
	float x = (float) luaL_checknumber(L, 2);
	float y = (float) luaL_checknumber(L, 3);
    ScriptEnvironment &se = GetScriptEnvironment(L);
    actor->MoveTo(se.world, vec2d(x,y));
	return 1;
}

// local x,y = getposition("object name")
static int object_getposition(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
	{
		return luaL_error(L, "1 argument expected; got %d", n);
	}

	GC_Actor *actor = checkobject<GC_Actor>(L, 1);
	lua_pushnumber(L, actor->GetPos().x);
	lua_pushnumber(L, actor->GetPos().y);

	return 2;
}

// type("object name")
static int object_type(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
	{
		return luaL_error(L, "1 argument expected; got %d", n);
	}

	GC_Object *obj = luaT_checkobject(L, 1);
	lua_pushstring(L, RTTypes::Inst().GetTypeName(obj->GetType()));
	return 1;
}

// equip("object name", "pickup name")
static int object_equip(lua_State *L)
{
	int n = lua_gettop(L);
	if( 2 != n )
	{
		return luaL_error(L, "2 arguments expected; got %d", n);
	}

	GC_Vehicle *target = checkobject<GC_Vehicle>(L, 1);
	GC_Pickup *pickup = checkobject<GC_Pickup>(L, 2);
    ScriptEnvironment &se = GetScriptEnvironment(L);
	
	if( pickup->GetAttached() )
		pickup->Detach(se.world);
	
	pickup->Attach(se.world, *target);
	
	return 0;
}

// ai_attack(player, x, y)
static int object_ai_march(lua_State *L)
{
	int n = lua_gettop(L);
	if( 3 != n )
	{
		return luaL_error(L, "3 arguments expected; got %d", n);
	}

	GC_Player *who = checkobject<GC_Player>(L, 1);
	float x = (float) luaL_checknumber(L, 2);
	float y = (float) luaL_checknumber(L, 3);

//	lua_pushboolean(L, who->March(x, y));
    return luaL_error(L, "not implemented");
	return 1;
}

// ai_attack(player, target)
static int object_ai_attack(lua_State *L)
{
	int n = lua_gettop(L);
	if( 2 != n )
	{
		return luaL_error(L, "2 argument expected; got %d", n);
	}

	GC_Player *who = checkobject<GC_Player>(L, 1);
	GC_RigidBodyStatic *what = checkobject<GC_RigidBodyStatic>(L, 2);

//	lua_pushboolean(L, who->Attack(what));
    return luaL_error(L, "not implemented");
	return 1;
}

static int object_ai_stop(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
	{
		return luaL_error(L, "1 argument expected; got %d", n);
	}

	GC_Player *ai = checkobject<GC_Player>(L, 1);
//	ai->Stop();
    return luaL_error(L, "not implemented");

	return 1;
}

// ai_pickup(player, target)
static int object_ai_pickup(lua_State *L)
{
	int n = lua_gettop(L);
	if( 2 != n )
	{
		return luaL_error(L, "2 argument expected; got %d", n);
	}

	GC_Player *who = checkobject<GC_Player>(L, 1);
	GC_Pickup *what = checkobject<GC_Pickup>(L, 2);

//	lua_pushboolean(L, who->Pickup(what));
    return luaL_error(L, "not implemented");
	return 1;
}

static const luaL_Reg objectlib[] = {
	{"damage", object_damage},
	{"kill", object_kill},
	{"setpos", object_setposition},
	{"getpos", object_getposition},
	{"type", object_type},
	{"equip", object_equip},
	{"ai_march", object_ai_march},
	{"ai_attack", object_ai_attack},
	{"ai_stop", object_ai_stop},
	{"ai_pickup", object_ai_pickup},
	{nullptr, nullptr}
};

static const luaL_Reg objectmt[] = {
	{"__gc", objfinalizer},
	{"__persist", objpersist},
	{"__newindex", pset},
	{"__index", pget},
	{"__next", objnextprop},
	{nullptr, nullptr}
};

int luaopen_object(lua_State *L)
{
	luaL_newmetatable(L, "object");
	luaL_register(L, nullptr, objectmt);
	lua_pop(L, 1); // pop the metatable
	luaL_register(L, "object", objectlib);
	return 1;
}
