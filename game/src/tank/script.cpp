// script.cpp

#include "stdafx.h"
#include "script.h"
#include "level.h"
#include "macros.h"

#include "gc/GameClasses.h"
#include "gc/vehicle.h"
#include "gc/pickup.h"
#include "gc/ai.h"

#include "ui/GuiManager.h"
#include "ui/gui_desktop.h"
#include "ui/gui.h"

#include "core/Console.h"
#include "core/debug.h"

#include "video/TextureManager.h"

#include "network/TankClient.h"
#include "network/TankServer.h"

#include "functions.h"


///////////////////////////////////////////////////////////////////////////////
// c closures

// exit to the system
static int luaT_quit(lua_State *L)
{
	if( g_level && !g_level->IsSafeMode() )
		return luaL_error(L, "attempt to execute 'quit' in unsafe mode");
	DestroyWindow(g_env.hMainWnd);
	return 0;
}

static int luaT_reset(lua_State *L)
{
	if( g_level && !g_level->IsSafeMode() )
		return luaL_error(L, "attempt to execute 'reset' in unsafe mode");

	SAFE_DELETE(g_level);
	SAFE_DELETE(g_client);
	SAFE_DELETE(g_server);


	//
	// clear the queue
	//

	lua_getglobal(L, "pushcmd");
	_ASSERT(LUA_TFUNCTION == lua_type(L, -1));
	lua_newtable(L);
	lua_setupvalue(L, -2, 1); // pops result of lua_newtable
	lua_pop(L, 1);  // pop result of lua_getglobal


	//
	// clear message area
	//

	static_cast<UI::Desktop*>(g_gui->GetDesktop())->GetMsgArea()->Clear();
	static_cast<UI::Desktop*>(g_gui->GetDesktop())->ShowEditor(false);

	return 0;
}

// msgbox(handler, "text", "btn1 text")
static int luaT_msgbox(lua_State *L)
{
	int n = lua_gettop(L);
	if( n > 5 || 2 > n )
		return luaL_error(L, "wrong number of arguments: 2-5 expected, got %d", n);

	luaL_checktype(L, 1, LUA_TFUNCTION);
	const char *text = luaL_checkstring(L, 2);
	const char *btn1 = lua_tostring(L, 3);
	const char *btn2 = lua_tostring(L, 4);
	const char *btn3 = lua_tostring(L, 5);

	lua_pushvalue(L, 1);
	int handler = luaL_ref(L, LUA_REGISTRYINDEX);

	new UI::ScriptMessageBox(g_gui->GetDesktop(), handler, text, btn1 ? btn1 : "OK", btn2, btn3);

	return 0;
}

// start/stop the timer
static int luaT_pause(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
		return luaL_error(L, "wrong number of arguments: 1 expected, got %d", n);

	luaL_checktype(L, 1, LUA_TBOOLEAN);

	PauseGame( 0 != lua_toboolean(L, 1) );

	return 0;
}

static int luaT_freeze(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
		return luaL_error(L, "wrong number of arguments: 1 expected, got %d", n);

	luaL_checktype(L, 1, LUA_TBOOLEAN);

	if( !g_level )
	{
		return luaL_error(L, "no game started");
	}

	g_level->Freeze( 0 != lua_toboolean(L, 1) );

	return 0;
}

// loadmap( string filename )
static int luaT_loadmap(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
		return luaL_error(L, "wrong number of arguments: 1 expected, got %d", n);

	const char *filename = luaL_checkstring(L, 1);

	if( g_level )
	{
		if( !g_level->IsSafeMode() )
			return luaL_error(L, "attempt to execute 'loadmap' in unsafe mode");
		delete g_level;
	}
	g_level = new Level();

	if( !g_level->init_newdm(filename, rand()) )
	{
		SAFE_DELETE(g_level);
		return luaL_error(L, "couldn't load map from '%s'", filename);
	}

	static_cast<UI::Desktop*>(g_gui->GetDesktop())->ShowEditor(false);

	return 0;
}

// newmap(int x_size, int y_size)
static int luaT_newmap(lua_State *L)
{
	int n = lua_gettop(L);
	if( 2 != n )
		return luaL_error(L, "wrong number of arguments: 2 expected, got %d", n);

	int x = __max(LEVEL_MINSIZE, __min(LEVEL_MAXSIZE, luaL_checkint(L, 1) ));
	int y = __max(LEVEL_MINSIZE, __min(LEVEL_MAXSIZE, luaL_checkint(L, 2) ));

	if( g_level )
	{
		if( !g_level->IsSafeMode() )
			return luaL_error(L, "attempt to execute 'newmap' in unsafe mode");
		delete g_level;
	}
	g_level = new Level();
	g_level->Init(x, y);
	if( !g_level->init_emptymap() )
	{
		SAFE_DELETE(g_level);
		return luaL_error(L, "couldn't create an empty map with the size %dx%d", x, y);
	}

	static_cast<UI::Desktop*>(g_gui->GetDesktop())->ShowEditor(true);

	return 0;
}

// load( string filename )  -- load a saved game
static int luaT_load(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
		return luaL_error(L, "wrong number of arguments: 1 expected, got %d", n);

	const char *filename = luaL_checkstring(L, 1);

	if( g_level )
	{
		if( !g_level->IsSafeMode() )
			return luaL_error(L, "attempt to execute 'load' in unsafe mode");
		delete g_level;
	}
	SAFE_DELETE(g_client);
	SAFE_DELETE(g_server);
	g_level = new Level();

	if( !g_level->init_load(filename) )
	{
		SAFE_DELETE(g_level);
		return luaL_error(L, "couldn't load game from '%s'", filename);
	}

	static_cast<UI::Desktop*>(g_gui->GetDesktop())->ShowEditor(false);

	return 0;
}

// save( string filename )  -- save game
static int luaT_save(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
		return luaL_error(L, "wrong number of arguments: 1 expected, got %d", n);

	const char *filename = luaL_checkstring(L, 1);

	if( !g_level )
	{
		return luaL_error(L, "no game started");
	}

	if( !g_level->IsSafeMode() )
		return luaL_error(L, "attempt to execute 'save' in unsafe mode");

	g_level->PauseSound(true);
	bool result = g_level->Serialize(filename);
	g_level->PauseSound(false);

	if( !result )
	{
		return luaL_error(L, "couldn't save game to '%s'", filename);
	}

	g_console->printf("game saved: '%s'\n", filename);

	return 0;
}

// import( string filename )  -- import map
static int luaT_import(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
		return luaL_error(L, "wrong number of arguments: 1 expected, got %d", n);

	const char *filename = luaL_checkstring(L, 1);

	if( g_level )
	{
		if( !g_level->IsSafeMode() )
			return luaL_error(L, "attempt to execute 'import' in unsafe mode");
		delete g_level;
	}
	SAFE_DELETE(g_client);
	SAFE_DELETE(g_server);
	g_level = new Level();

	if( !g_level->init_import_and_edit(filename) )
	{
		SAFE_DELETE(g_level);
		return luaL_error(L, "couldn't import map '%s'", filename);
	}

	static_cast<UI::Desktop*>(g_gui->GetDesktop())->ShowEditor(true);

	return 0;
}

// export( string filename )  -- export map
static int luaT_export(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
		return luaL_error(L, "wrong number of arguments: 1 expected, got %d", n);

	const char *filename = luaL_checkstring(L, 1);

	if( !g_level )
	{
		return luaL_error(L, "no map loaded");
	}

	if( !g_level->IsSafeMode() )
		return luaL_error(L, "attempt to execute 'export' in unsafe mode");

	bool result = g_level->Export(filename);

	if( !result )
	{
		return luaL_error(L, "couldn't export map to '%s'", filename);
	}

	g_console->printf("map exported: '%s'\n", filename);

	return 0;
}

// print a message to the MessageArea
static int luaT_message(lua_State *L)
{
	int n = lua_gettop(L);        // number of arguments
	lua_getglobal(L, "tostring");
	std::ostringstream buf;
	for( int i = 1; i <= n; i++ )
	{
		const char *s;
		lua_pushvalue(L, -1);     // function to be called
		lua_pushvalue(L, i);      // value to print
		lua_call(L, 1, 1);
		s = lua_tostring(L, -1);  // get result
		if( NULL == s )
			return luaL_error(L, LUA_QL("tostring") " must return a string to " LUA_QL("print"));
		buf << s;
		lua_pop(L, 1);            // pop result
	}
	static_cast<UI::Desktop*>(g_gui->GetDesktop())->GetMsgArea()->puts(buf.str().c_str());
	return 0;
}

static int luaT_print(lua_State *L)
{
	int n = lua_gettop(L);     // get number of arguments
	lua_getglobal(L, "tostring");
	for( int i = 1; i <= n; ++i )
	{
		lua_pushvalue(L, -1);  // function to be called
		lua_pushvalue(L, i);   // value to print (1-st arg)
		lua_call(L, 1, 1);
		const char *s = lua_tostring(L, -1);  // get result string
		if( NULL == s )
		{
			return luaL_error(L, LUA_QL("tostring") " must return a string to " LUA_QL("print"));
		}
		g_console->puts(s);
		lua_pop(L, 1);         // pop result
	}
	g_console->puts("\n");
	return 0;
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

static void safe_tostr(lua_State *L, char *dst, size_t bufsize)
{
	size_t len;
	const char *str = lua_tolstring(L, -1, &len);
	len = __min(bufsize - 1, len);
	memcpy(dst, str, len);
	dst[len] = 0;
}

// convert vehicle class
int luaT_ConvertVehicleClass(lua_State *L)
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

	lua_getfield(L, 1, "bounds");
	if( !lua_istable(L, -1) )
	{
		return luaL_error(L, "absent mandatory table field 'bounds'");
	}

	for( int i = 1; i <= 4; i++ )
	{
		lua_pushinteger(L, i); // push key
		lua_gettable(L, -2);   // pop key and push value
		if( !lua_istable(L, -1) )
		{
			return luaL_error(L, "table expected at bounds[%d]", i);
		}
		get_array(L, (float*) (vc.bounds + i-1), 2);
		lua_pop(L, 1); // pop result of gettable
	}

	lua_pop(L, 1); // pop result of getfield(bounds)


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


// prop name at -2; prop value at -1
// return false if property not found
int pset_helper(const SafePtr<PropertySet> &properties, lua_State *L)
{
	const char *pname = lua_tostring(L, -2);

	ObjectProperty *p = NULL;

	for( int i = 0; i < properties->GetCount(); ++i )
	{
		p = properties->GetProperty(i);
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
		_ASSERT(FALSE);
	}

	return 1;
}

// actor("type name", x, y [, params])
int luaT_actor(lua_State *L)
{
	int n = lua_gettop(L);
	if( 3 != n && 4 != n )
	{
		return luaL_error(L, "3 or 4 arguments expected; got %d", n);
	}

	const char *name = luaL_checkstring(L, 1);
	float x = (float) luaL_checknumber(L, 2);
	float y = (float) luaL_checknumber(L, 3);


	if( !g_level )
	{
		return luaL_error(L, "no game started");
	}

	ObjectType type = Level::GetTypeByName(name);
	if( INVALID_OBJECT_TYPE == type )
	{
		return luaL_error(L, "unknown type '%s'", name);
	}

	if( Level::GetTypeInfo(type).service )
	{
		return luaL_error(L, "type '%s' is a service", name);
	}

	GC_Object *obj = g_level->CreateObject(type, x, y);


	if( 4 == n )
	{
		luaL_checktype(L, 4, LUA_TTABLE);

		SafePtr<PropertySet> properties = obj->GetProperties();
		_ASSERT(properties);

		for( lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1) )
		{
			// now 'key' is at index -2 and 'value' at index -1
			pset_helper(properties, L);
		}

		properties->Exchange(true);
	}

	return 0;
}

// service("type name" [, params])
int luaT_service(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n && 2 != n )
	{
		return luaL_error(L, "1 or 2 arguments expected; got %d", n);
	}

	const char *name = luaL_checkstring(L, 1);

	if( !g_level )
	{
		return luaL_error(L, "no game started");
	}

	ObjectType type = Level::GetTypeByName(name);
	if( INVALID_OBJECT_TYPE == type )
	{
		return luaL_error(L, "unknown type '%s'", name);
	}

	if( !Level::GetTypeInfo(type).service )
	{
		return luaL_error(L, "type '%s' is not a service", name);
	}

	GC_Object *obj = g_level->CreateObject(type, 0, 0);


	if( 2 == n )
	{
		luaL_checktype(L, 2, LUA_TTABLE);

		SafePtr<PropertySet> properties = obj->GetProperties();
		_ASSERT(properties);

		for( lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1) )
		{
			// now 'key' is at index -2 and 'value' at index -1
			pset_helper(properties, L);
		}

		properties->Exchange(true);
	}

	return 0;
}


// damage(hp, "victim")
int luaT_damage(lua_State *L)
{
	int n = lua_gettop(L);
	if( 2 != n )
	{
		return luaL_error(L, "2 arguments expected; got %d", n);
	}

	float hp = (float) luaL_checknumber(L, 1);
	const char *name = luaL_checkstring(L, 2);

	if( !g_level )
	{
		return luaL_error(L, "no game started");
	}

	GC_Object *obj = g_level->FindObject(name);

	if( NULL == obj )
	{
		return luaL_error(L, "object with name '%s' was not found", name);
	}

	GC_RigidBodyStatic *rbs = dynamic_cast<GC_RigidBodyStatic *>(obj);
	if( NULL == rbs )
	{
		return luaL_error(L, "object '%s' couldn't be damaged");
	}

	rbs->TakeDamage(hp, rbs->GetPos(), NULL);

	return 0;
}

// kill("object name")
int luaT_kill(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
	{
		return luaL_error(L, "1 argument expected; got %d", n);
	}

	const char *name = luaL_checkstring(L, 1);

	if( !g_level )
	{
		return luaL_error(L, "no game started");
	}

	GC_Object *obj = g_level->FindObject(name);
	if( NULL == obj )
	{
		return luaL_error(L, "object with name '%s' was not found", name);
	}
	_ASSERT(!obj->IsKilled());

	obj->Kill();

	return 0;
}


// exists("object name")
int luaT_exists(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
	{
		return luaL_error(L, "1 argument expected; got %d", n);
	}

	const char *name = luaL_checkstring(L, 1);

	if( !g_level )
	{
		return luaL_error(L, "no game started");
	}

	GC_Object *obj = g_level->FindObject(name);
	_ASSERT(!obj->IsKilled());

	lua_pushboolean(L, NULL != obj);
	return 1;
}



// pget("object name", "property name")
int luaT_pget(lua_State *L)
{
	int n = lua_gettop(L);
	if( 2 != n )
	{
		return luaL_error(L, "2 arguments expected; got %d", n);
	}

	const char *name = luaL_checkstring(L, 1);
	const char *prop = luaL_checkstring(L, 2);

	if( !g_level )
	{
		return luaL_error(L, "no game started");
	}

	GC_Object *obj = g_level->FindObject(name);

	if( NULL == obj )
	{
		return luaL_error(L, "object with name '%s' was not found", name);
	}
	_ASSERT(!obj->IsKilled());

	SafePtr<PropertySet> properties = obj->GetProperties();
	_ASSERT(properties);

	for( int i = 0; i < properties->GetCount(); ++i )
	{
		ObjectProperty *p = properties->GetProperty(i);
		if( p->GetName() == prop )
		{
			switch( p->GetType() )
			{
			case ObjectProperty::TYPE_INTEGER:
				lua_pushinteger(L, p->GetIntValue());
				break;
			case ObjectProperty::TYPE_FLOAT:
				lua_pushnumber(L, p->GetFloatValue());
				break;
			case ObjectProperty::TYPE_STRING:
				lua_pushstring(L, p->GetStringValue().c_str());
				break;
			case ObjectProperty::TYPE_MULTISTRING:
				lua_pushstring(L, p->GetListValue(p->GetCurrentIndex()).c_str());
				break;
			default:
				_ASSERT(FALSE);
			}
			return 1;
		}
	}

	return luaL_error(L, "object '%s' has no property '%s'", name, prop);
}


// pset("object name", "property name", value)
int luaT_pset(lua_State *L)
{
	int n = lua_gettop(L);
	if( 3 != n )
	{
		return luaL_error(L, "3 arguments expected; got %d", n);
	}

	const char *name = luaL_checkstring(L, 1);
	const char *prop = luaL_checkstring(L, 2);
	luaL_checkany(L, 3);  // prop value should be here

	if( !g_level )
	{
		return luaL_error(L, "no game started");
	}

	GC_Object *obj = g_level->FindObject(name);
	if( NULL == obj )
	{
		return luaL_error(L, "object with name '%s' was not found", name);
	}
	_ASSERT(!obj->IsKilled());

	SafePtr<PropertySet> properties = obj->GetProperties();
	_ASSERT(properties);

	// prop name at -2; prop value at -1
	if( !pset_helper(properties, L) )
	{
		return luaL_error(L, "object '%s' has no property '%s'", name, prop);
	}

	properties->Exchange(true);
	return 0;
}

// equip("object name", "pickup name")
int luaT_equip(lua_State *L)
{
	int n = lua_gettop(L);
	if( 2 != n )
	{
		return luaL_error(L, "2 arguments expected; got %d", n);
	}
	const char *targetname = luaL_checkstring(L, 1);
	const char *pickupname = luaL_checkstring(L, 2);

	if( !g_level )
	{
		return luaL_error(L, "no game started");
	}

	GC_Object *target_raw = g_level->FindObject(targetname);
	if( NULL == target_raw )
	{
		return luaL_error(L, "object with name '%s' was not found", targetname);
	}

	GC_Object *pickup_raw = g_level->FindObject(pickupname);
	if( NULL == target_raw )
	{
		return luaL_error(L, "object with name '%s' was not found", pickupname);
	}

	GC_Pickup *pickup = dynamic_cast<GC_Pickup *>(pickup_raw);
	if( NULL == pickup )
	{
		return luaL_error(L, "object '%s' is not a pickup", pickupname);
	}

	GC_Vehicle *target = dynamic_cast<GC_Vehicle *>(target_raw);
	if( NULL == target )
	{
		return luaL_error(L, "target object '%s' is not a vehicle", targetname);
	}

	if( pickup->IsAttached() )
	{
		pickup->Detach();
	}
	pickup->Attach(target);

	return 0;
}

int luaT_loadtheme(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
	{
		return luaL_error(L, "1 argument expected; got %d", n);
	}

	const char *filename = luaL_checkstring(L, 1);

	if( 0 == g_texman->LoadPackage(filename) )
	{
		g_console->puts("WARNING: there are no textures loaded\n");
	}

	return 0;
}

int luaT_pushcmd(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TFUNCTION);
	if( 1 == lua_gettop(L) )
		lua_pushnumber(L, 0);
	luaL_checktype(L, 2, LUA_TNUMBER);
	lua_settop(L, 2);

	lua_createtable(L, 2, 0); // create a new table at index 3
	lua_pushvalue(L, 1);      // push copy of the function
	lua_rawseti(L, 3, 1);
	lua_pushvalue(L, 2);      // push copy of the delay
	lua_rawseti(L, 3, 2);

	lua_rawseti(L, lua_upvalueindex(1), lua_objlen(L, lua_upvalueindex(1)) + 1);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// api

lua_State* script_open(void)
{
	lua_State *L = luaL_newstate();


	//
	// open std libs
	//

	static const luaL_Reg lualibs[] = {
		{"", luaopen_base},
		{LUA_LOADLIBNAME, luaopen_package},
		{LUA_TABLIBNAME, luaopen_table},
//		{LUA_IOLIBNAME, luaopen_io},
//		{LUA_OSLIBNAME, luaopen_os},
		{LUA_STRLIBNAME, luaopen_string},
		{LUA_MATHLIBNAME, luaopen_math},
#ifdef _DEBUG
		{LUA_DBLIBNAME, luaopen_debug},
#endif
		{NULL, NULL}
	};

	for( const luaL_Reg *lib = lualibs; lib->func; ++lib )
	{
		lua_pushcfunction(L, lib->func);
		lua_pushstring(L, lib->name);
		lua_call(L, 1, 0);
	}



	//
	// register functions
	//

	lua_register(L, "loadmap",  luaT_loadmap);
	lua_register(L, "newmap",   luaT_newmap);
	lua_register(L, "load",     luaT_load);
	lua_register(L, "save",     luaT_save);
	lua_register(L, "import",   luaT_import);
	lua_register(L, "export",   luaT_export);
	lua_register(L, "loadtheme",luaT_loadtheme);

	lua_register(L, "actor",    luaT_actor);
	lua_register(L, "service",  luaT_service);

	lua_register(L, "damage",   luaT_damage);
	lua_register(L, "kill",     luaT_kill);
	lua_register(L, "pget",     luaT_pget);
	lua_register(L, "pset",     luaT_pset);
	lua_register(L, "equip",    luaT_equip);
	lua_register(L, "exists",   luaT_exists);

	lua_register(L, "msgbox",   luaT_msgbox);

	lua_register(L, "message",  luaT_message);
	lua_register(L, "print",    luaT_print);

	lua_register(L, "reset",    luaT_reset);
	lua_register(L, "quit",     luaT_quit);
	lua_register(L, "pause",    luaT_pause);
	lua_register(L, "freeze",   luaT_freeze);


	//
	// init the command queue
	//
	lua_newtable(L);
	lua_pushcclosure(L, luaT_pushcmd, 1);
	lua_setglobal(L, "pushcmd");


	//
	// create global 'gc' table and fill it with editor classes
	//

	lua_newtable(L);
	for( int i = 0; i < Level::GetTypeCount(); ++i )
	{
		lua_newtable(L);
		lua_setfield(L, -2, Level::GetTypeInfoByIndex(i).name);
	}
	lua_setglobal(L, "gc"); // set global and pop one element from stack


	//
	// create global 'classes' table
	//

	lua_newtable(L);
	lua_setglobal(L, "classes"); // set global and pop one element from stack


	//
	// create global 'user' table
	//

	lua_newtable(L);
	lua_setglobal(L, "user"); // set global and pop one element from stack


	return L;
}

void script_close(lua_State *L)
{
	_ASSERT(L);
	lua_close(L);
}

bool script_exec(lua_State *L, const char *string)
{
	_ASSERT(L);

	if( luaL_loadstring(L, string) )
	{
		g_console->printf("syntax error %s\n", lua_tostring(L, -1));
		lua_pop(L, 1); // pop the error message from the stack
		return false;
	}

	if( lua_pcall(L, 0, 0, 0) )
	{
		g_console->printf("%s\n", lua_tostring(L, -1));
		lua_pop(L, 1); // pop the error message from the stack
		return false;
	}

	return true;
}

bool script_exec_file(lua_State *L, const char *filename)
{
	_ASSERT(L);

	if( luaL_loadfile(L, filename) )
	{
		TRACE("%s\n", lua_tostring(L, -1));
		return false;
	}

	if( lua_pcall(L, 0, 0, 0) )
	{
		TRACE("runtime error: %s\n", lua_tostring(L, -1));
		lua_pop(L, 1); // pop the error message from the stack
		return false;
	}

	return true;
}


///////////////////////////////////////////////////////////////////////////////
// end of file
