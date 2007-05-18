// script.cpp

#include "stdafx.h"
#include "script.h"
#include "level.h"
#include "macros.h"

#include "gc/GameClasses.h"
#include "gc/vehicle.h"
#include "gc/player.h"
#include "gc/editor.h"

#include "core/Console.h"
#include "core/debug.h"

///////////////////////////////////////////////////////////////////////////////
// c closures

// exit to the system
static int luaT_quit(lua_State *L)
{
	DestroyWindow(g_env.hMainWnd);
	return 0;
}

// start/stop the timer
static int luaT_pause(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
		return luaL_error(L, "wrong number of arguments: 1 expected, got %d", n);

	luaL_checktype(L, 1, LUA_TBOOLEAN);

	if( !g_level )
	{
		return luaL_error(L, "no game started");
	}

	g_level->Pause( 0 != lua_toboolean(L, 1) );

	return 0;
}

// loadmap( string filename )
static int luaT_loadmap(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
		return luaL_error(L, "wrong number of arguments: 1 expected, got %d", n);

	const char *filename = luaL_checkstring(L, 1);

	SAFE_DELETE(g_level);
	g_level = new Level();

	if( !g_level->init_newdm(filename) )
	{
		SAFE_DELETE(g_level);
		return luaL_error(L, "couldn't load map from '%s'", filename);
	}

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

	SAFE_DELETE(g_level);
	g_level = new Level();
	g_level->Init(x, y);
    if( !g_level->init_emptymap() )
	{
		SAFE_DELETE(g_level);
		return luaL_error(L, "couldn't create an empty map with the size %dx%d", x, y);
	}
	
	return 0;
}

// load( string filename )  -- load a saved game
static int luaT_load(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
		return luaL_error(L, "wrong number of arguments: 1 expected, got %d", n);

	const char *filename = luaL_checkstring(L, 1);

	SAFE_DELETE(g_level);
	g_level = new Level();

	if( !g_level->init_load(filename) )
	{
		SAFE_DELETE(g_level);
		return luaL_error(L, "couldn't load game from '%s'", filename);
	}

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

	g_level->Pause(true);
	bool result = g_level->Serialize(filename);
	g_level->Pause(false);

	if( !result )
	{
		return luaL_error(L, "couldn't save game to '%s'", filename);
	}

	return 0;
}

// print a message to the MessageArea
static int luaT_message(lua_State *L)
{
	_ASSERT(_MessageArea::Inst());

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
	_MessageArea::Inst()->message(buf.str().c_str());
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
	vc.display_name = lua_isstring(L, -1) ? lua_tostring(L, -1) : "<unnamed>";
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
	vc.engine_power = tmp[0];
	vc.rotate_power = tmp[1];


	//
	// get mass, inertia, etc
	//

	get_numeric(L, "mass",    vc.m);
	get_numeric(L, "inertia", vc.i);
	get_numeric(L, "health",  vc.health);
	get_numeric(L, "percussion", vc.percussion);
	get_numeric(L, "fragility",  vc.fragility);

	return 0;
}


//
// SYNOPSIS:
//  addplayer{
//      name = <string>,
//      type = <number>,
//      team = <number>,
//      skin = <string>,
//      cls  = <string> }
//
static int luaT_addplayer(lua_State *L)
{
	PlayerDesc pd = {0};

	if( !g_level )
		return luaL_error(L, "no map loaded");

	//
	// check args
	//
	int n = lua_gettop(L);
	if( 1 != n )
	{
		return luaL_error(L, "one argument expected; got %d", n);
	}
	luaL_checktype(L, 1, LUA_TTABLE);

	//-------------

	lua_getfield(L, 1, "name");
	if( lua_isstring(L, -1) )
	{
		safe_tostr(L, pd.name, MAX_PLRNAME);
	}
	else
	{
        lua_getglobal(L, "random_names");
		lua_pushinteger(L, rand() % lua_objlen(L, -1) + 1);  // push key
		lua_gettable(L, -2);                                 // pop key, push value
		safe_tostr(L, pd.name, MAX_PLRNAME);                 // get value
		lua_pop(L, 2);                                       // pop value and table                
	}
	lua_pop(L, 1); // pop result of lua_getfield

	//-------------

	lua_getfield(L, 1, "cls");
	safe_tostr(L, pd.cls, MAX_VEHCLSNAME);   // get vehicle class
	lua_pop(L, 1);                           // pop result of lua_getfield

    lua_getglobal(L, "classes");
	lua_getfield(L, -1, pd.cls);
	if( lua_isnil(L, -1) )
	{
		luaL_error(L, "vehicle class '%s' doesn't exist", pd.cls);
	}
	lua_pop(L, 2); // pop result of lua_getglobal and lua_getfield

	//-------------

	lua_getfield(L, 1, "skin");
	safe_tostr(L, pd.skin, MAX_PATH);   // get skin name
	lua_pop(L, 1);                      // pop result of lua_getfield

    //-------------

	lua_getfield(L, 1, "team");
	pd.team = lua_tointeger(L, -1);
	lua_pop(L, 1); // pop result of lua_getfield

	//-------------

	lua_getfield(L, 1, "type");
	if( lua_isnumber(L, -1) )
		pd.type = lua_tointeger(L, -1);
	else
		pd.type = MAX_HUMANS;
	lua_pop(L, 1); // pop result of lua_getfield

	//-------------

	//
	// add player
	//
	GC_Player *player = new GC_Player(pd.team);
	player->SetController(pd.type);
	strcpy(player->_name,  pd.name);
	strcpy(player->_skin,  pd.skin);
	strcpy(player->_class, pd.cls);

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// api

script_h script_open(void)
{
	lua_State *L = lua_open();

	//
	// register functions
	//

	luaopen_base(L);

	lua_register(L, "loadmap",   luaT_loadmap);
	lua_register(L, "newmap",    luaT_newmap);
	lua_register(L, "load",      luaT_load);
	lua_register(L, "save",      luaT_save);

	lua_register(L, "addplayer", luaT_addplayer);

	lua_register(L, "message",   luaT_message);
	lua_register(L, "print",     luaT_print);

	lua_register(L, "quit",      luaT_quit);
	lua_register(L, "pause",     luaT_pause);


	//
	// create global 'gc' table and fill it with editor classes
	//

	lua_newtable(L);
	lua_pushvalue(L, -1);   // make a copy of the table to leave it in the stack
	lua_setglobal(L, "gc"); // set global and pop one element from stack

	for( int i = 0; i < _Editor::Inst()->GetObjectCount(); ++i )
	{
		const char *cls = _Editor::Inst()->GetName(_Editor::Inst()->GetOwnedType(i));
        lua_newtable(L);
		lua_setfield(L, -2, cls);
	}

	lua_pop(L, 1); // remove 'gc' table from the stack


	//
	// create global 'classes' table
	//

	lua_newtable(L);
	lua_setglobal(L, "classes"); // set global and pop one element from stack

	//////////////////////////////////////
	return L;
}

void script_close(script_h s)
{
	_ASSERT(s);
    lua_close(LS(s));
}

bool script_exec(script_h s, const char *string)
{
	_ASSERT(s);

	if( luaL_loadstring(LS(s), string) )
	{
		g_console->printf("syntax error %s\n", lua_tostring(LS(s), -1));
		return false;
	}

	if( lua_pcall(LS(s), 0, 0, 0) )
	{
		g_console->printf("%s\n", lua_tostring(LS(s), -1));
		return false;
	}

    return true;
}

bool script_exec_file(script_h s, const char *filename)
{
	_ASSERT(s);

	if( luaL_loadfile(LS(s), filename) )
	{
		TRACE("%s\n", lua_tostring(LS(s), -1));
		return false;
	}

	if( lua_pcall(LS(s), 0, 0, 0) )
	{
		TRACE("runtime error: %s\n", lua_tostring(LS(s), -1));
		return false;
	}

    return true;
}


///////////////////////////////////////////////////////////////////////////////
// end of file
