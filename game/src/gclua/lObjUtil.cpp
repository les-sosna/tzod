#include "inc/gclua/lObjUtil.h"
#include <gc/Object.h>
#include <gc/ObjPtr.h>
extern "C"
{
#include <lua.h>
#include <lauxlib.h>
}

World& luaT_getworld(lua_State *L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, "WORLD");
    void *ud = lua_touserdata(L, -1);
    assert(ud);
    lua_pop(L, 1);
    return *reinterpret_cast<World*>(ud);
}

void luaT_pushobject(lua_State *L, GC_Object *obj)
{
	ObjPtr<GC_Object> *ppObj = (ObjPtr<GC_Object> *) lua_newuserdata(L, sizeof(ObjPtr<GC_Object>));
	luaL_getmetatable(L, "object");
	lua_setmetatable(L, -2);
	new (ppObj) ObjPtr<GC_Object>(obj);
}

GC_Object* luaT_checkobject(lua_State *L, int n)
{
	ObjPtr<GC_Object> *ppObj = (ObjPtr<GC_Object> *) luaL_checkudata(L, n, "object");
	if( !*ppObj )
		luaL_argerror(L, n, "reference to dead object");
	return *ppObj;
}

// prop name at -2; prop value at -1; return 0 if property not found
int luaT_setproperty(lua_State *L, PropertySet &properties)
{
	const char *pname = lua_tostring(L, -2);

	ObjectProperty *p = NULL;

	for( int i = 0; i < properties.GetCount(); ++i )
	{
		p = properties.GetProperty(i);
		if( p->GetName() == pname )
		{
			break;
		}
		p = NULL;
	}

	if( NULL == p )
	{
		return 0;  // property not found
	}


	switch( p->GetType() )
	{
		case ObjectProperty::TYPE_INTEGER:
		{
			if( LUA_TNUMBER != lua_type(L, -1) )
			{
				luaL_error(L, "property '%s' - expected integer value; got %s",
						   pname, lua_typename(L, lua_type(L, -1)));
			}
			int v = lua_tointeger(L, -1);
			if( v < p->GetIntMin() || v > p->GetIntMax() )
			{
				return luaL_error(L, "property '%s' - value %d is out of range [%d, %d]",
								  pname, v, p->GetIntMin(), p->GetIntMax());
			}
			p->SetIntValue(v);
			break;
		}
		case ObjectProperty::TYPE_FLOAT:
		{
			if( LUA_TNUMBER != lua_type(L, -1) )
			{
				luaL_error(L, "property '%s' - expected number value; got %s",
						   pname, lua_typename(L, lua_type(L, -1)));
			}
			float v = (float) lua_tonumber(L, -1);
			if( v < p->GetFloatMin() || v > p->GetFloatMax() )
			{
				return luaL_error(L, "property '%s' - value %g is out of range [%g, %g]",
								  pname, v, p->GetFloatMin(), p->GetFloatMax());
			}
			p->SetFloatValue(v);
			break;
		}
		case ObjectProperty::TYPE_STRING:
		case ObjectProperty::TYPE_SKIN:
		case ObjectProperty::TYPE_TEXTURE:
		{
			if( LUA_TSTRING != lua_type(L, -1) )
			{
				luaL_error(L, "property '%s' - expected string value; got %s",
						   pname, lua_typename(L, lua_type(L, -1)));
			}
			p->SetStringValue(lua_tostring(L, -1));
			break;
		}
		case ObjectProperty::TYPE_MULTISTRING:
		{
			if( LUA_TSTRING != lua_type(L, -1) )
			{
				luaL_error(L, "property '%s' - expected string value; got %s",
						   pname, lua_typename(L, lua_type(L, -1)));
			}
			const char *v = lua_tostring(L, -1);
			bool ok = false;
			for( size_t i = 0; i < p->GetListSize(); ++i )
			{
				if( p->GetListValue(i) == v )
				{
					p->SetCurrentIndex(i);
					ok = true;
					break;
				}
			}
			if( !ok )
			{
				return luaL_error(L, "property '%s' - attempt to set invalid value '%s'", pname, v);
			}
			break;
		}
		default:
			assert(false);
	}

	return 1;
}
