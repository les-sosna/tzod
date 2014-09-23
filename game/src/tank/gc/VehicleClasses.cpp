#include "VehicleClasses.h"
#include "globals.h"
#include "core/Debug.h"

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
}


static int get_array(lua_State *L, float *array, int count)
{
	for( int i = 0; i < count; i++ )
	{
		lua_pushinteger(L, i+1); // push key
		lua_gettable(L, -2);     // pop key, push value
		if( !lua_isnumber(L, -1) )
		{
			return luaL_error(L, "number expected in array");
		}
		array[i] = (float) lua_tonumber(L, -1);
		lua_pop(L, 1); // pop value
	}
	return 0;
}

static int get_numeric(lua_State *L, const char *field, float &refval)
{
	lua_getfield(L, 1, field);
	if( !lua_isnumber(L, -1) )
		return luaL_error(L, "absent mandatory numeric '%s' field", field);
	refval = (float) lua_tonumber(L, -1);
	lua_pop(L, 1); // pop result of getfield
	return 0;
}

// convert vehicle class
static int ConvertVehicleClass(lua_State *L)
{
	//
	//  validate arguments
	//
	
	if( 2 != lua_gettop(L) )
	{
		return luaL_error(L, "two arguments expected");
	}
	luaL_checktype(L, 1, LUA_TTABLE);
	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);
	
	
	VehicleClass &vc = *reinterpret_cast<VehicleClass *>(lua_touserdata(L, 2));
	
	
	//
	// get display name
	//
	lua_getfield(L, 1, "display");
	vc.displayName = lua_isstring(L, -1) ? lua_tostring(L, -1) : "<unnamed>";
	lua_pop(L, 1); // pop result of lua_getfield
	
	
	
	//
	// get bounds
	//
	
	get_numeric(L, "width",   vc.width);
	get_numeric(L, "length",  vc.length);
	
	
	//
	// get friction settings
	//
	
	float tmp[3];
	
	lua_getfield(L, 1, "dry_fric");
	if( !lua_istable(L, -1) )
	{
		return luaL_error(L, "absent mandatory array field 'dry_fric'");
	}
	get_array(L, tmp, 3);
	lua_pop(L, 1); // pop result of getfield;
	vc._Nx = tmp[0];
	vc._Ny = tmp[1];
	vc._Nw = tmp[2];
	
	lua_getfield(L, 1, "vis_fric");
	if( !lua_istable(L, -1) )
	{
		return luaL_error(L, "absent mandatory array field 'vis_fric'");
	}
	get_array(L, tmp, 3);
	lua_pop(L, 1); // pop result of getfield;
	vc._Mx = tmp[0];
	vc._My = tmp[1];
	vc._Mw = tmp[2];
	
	
	//
	// get power
	//
	
	lua_getfield(L, 1, "power");
	if( !lua_istable(L, -1) )
	{
		return luaL_error(L, "absent mandatory array field 'power'");
	}
	get_array(L, tmp, 2);
	lua_pop(L, 1); // pop result of getfield;
	vc.enginePower = tmp[0];
	vc.rotatePower = tmp[1];
	
	
	//
	// get max speed
	//
	
	lua_getfield(L, 1, "max_speed");
	if( !lua_istable(L, -1) )
	{
		return luaL_error(L, "absent mandatory array field 'max_speed'");
	}
	get_array(L, tmp, 2);
	lua_pop(L, 1); // pop result of getfield;
	vc.maxLinSpeed = tmp[0];
	vc.maxRotSpeed = tmp[1];
	
	
	//
	// get mass, inertia, etc
	//
	
	get_numeric(L, "mass",       vc.m);
	get_numeric(L, "inertia",    vc.i);
	get_numeric(L, "health",     vc.health);
	get_numeric(L, "percussion", vc.percussion);
	get_numeric(L, "fragility",  vc.fragility);
	
	return 0;
}


std::shared_ptr<const VehicleClass> GetVehicleClass(const char *className)
{
	lua_State *L = g_env.L;
	lua_pushcfunction(L, ConvertVehicleClass);     // func convert
	
	lua_getglobal(L, "getvclass");                      // func getvclass
	lua_pushstring(L, className);                       // clsname
	if( lua_pcall(L, 1, 1, 0) )                         // cls = getvclass(clsname)
	{
		// print error message
		GetConsole().WriteLine(1, lua_tostring(L, -1));
		lua_pop(L, 1);
		return nullptr;
	}
	
	auto vc = std::make_shared<VehicleClass>();
	lua_pushlightuserdata(L, vc.get());                 // &vc
	if( lua_pcall(L, 2, 0, 0) )                         // convert(cls, &vc)
	{
		// print error message
		GetConsole().WriteLine(1, lua_tostring(L, -1));
		lua_pop(L, 1);
		return nullptr;
	}
	return std::move(vc);
}
