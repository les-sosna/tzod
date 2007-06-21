// script.cpp

#include "stdafx.h"
#include "script.h"
#include "level.h"
#include "macros.h"

#include "gc/GameClasses.h"
#include "gc/vehicle.h"
#include "gc/ai.h"

#include "core/Console.h"
#include "core/debug.h"

#include "video/TextureManager.h"


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

	if( g_level )
	{
		if( !g_level->IsSafeMode() )
			return luaL_error(L, "attempt to execute 'loadmap' in unsafe mode");
		delete g_level;
	}
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

	if( !g_level->IsSafeMode() )
		return luaL_error(L, "attempt to execute 'save' in unsafe mode");

	g_level->Pause(true);
	bool result = g_level->Serialize(filename);
	g_level->Pause(false);

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
	g_level = new Level();

	if( !g_level->init_import_and_edit(filename) )
	{
		SAFE_DELETE(g_level);
		return luaL_error(L, "couldn't import map '%s'", filename);
	}

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
//  addbot{
//      nick   = <string>,
//      team   = <number>,
//      skin   = <string>,
//      class  = <string> }
//      level  = 0..4
//
static int luaT_addbot(lua_State *L)
{
	if( !g_level )
		return luaL_error(L, "no map loaded");

	//
	// check args
	//
	int n = lua_gettop(L);
	if( n > 1 )
	{
		return luaL_error(L, "one argument expected; got %d", n);
	}

	if( 0 == n )
	{
		lua_createtable(L, 0, 0);
	}

	luaL_checktype(L, 1, LUA_TTABLE);

	if( !g_level )
	{
		return luaL_error(L, "no game started");
	}


	//
	// add bot
	//

	GC_PlayerAI *player = new GC_PlayerAI();

	//--------------------

	lua_getfield(L, 1, "nick");
	if( lua_isstring(L, -1) )
	{
		player->SetNick(lua_tostring(L, -1));
	}
	else
	{
		// select name from the random_names table
		lua_getglobal(L, "random_names");                    // push table
		lua_pushinteger(L, rand() % lua_objlen(L, -1) + 1);  // push key
		lua_gettable(L, -2);                                 // pop key, push value
		player->SetNick(lua_tostring(L, -1));                // get value
		lua_pop(L, 2);                                       // pop value and table
	}
	lua_pop(L, 1); // pop result of lua_getfield

	//-------------

	lua_getfield(L, 1, "class");
	if( lua_isstring(L, -1) )
	{
		player->SetClass(lua_tostring(L, -1));  // get vehicle class
	}
	else
	{
		// select random class
		int count = 0;
		lua_getglobal(L, "classes");
		for( lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1) )
		{
			if( 0 == rand() % ++count )
			{
				player->SetClass(lua_tostring(L, -2));  // get vehicle class
			}
		}
	}
	lua_pop(L, 1);                           // pop result of lua_getfield

	//-------------

	lua_getfield(L, 1, "skin");
	if( lua_isstring(L, -1) )
	{
		player->SetSkin(lua_tostring(L, -1));  // get skin name
	}
	else
	{
		// select random skin
		std::vector<string_t> skins;
		g_texman->GetTextureNames(skins, "skin/", true);
		player->SetSkin(skins[rand() % skins.size()]);
	}
	lua_pop(L, 1);                      // pop result of lua_getfield

	//-------------

	lua_getfield(L, 1, "team");
	player->SetTeam(lua_tointeger(L, -1));
	lua_pop(L, 1); // pop result of lua_getfield

	//-------------

	lua_getfield(L, 1, "level");
	player->SetLevel(__min(4, __max(0, lua_tointeger(L, -1))));
	lua_pop(L, 1); // pop result of lua_getfield


	return 0;
}

// actor("type name", x, y, [params])
int luaT_actor(lua_State *L)
{
	int n = lua_gettop(L);
	if( 3 != n )
	{
		return luaL_error(L, "3 arguments expected; got %d", n);
	}

	const char *name = luaL_checkstring(L, 1);
	float x = (float) luaL_checknumber(L, 2);
	float y = (float) luaL_checknumber(L, 3);

	if( !g_level )
	{
		return luaL_error(L, "no game started");
	}

	for( int i = 0; i < Level::GetTypeCount(); ++i )
	{
		if( 0 == strcmp(name, Level::GetTypeInfo(i).name) )
		{
			g_level->CreateObject(Level::GetType(i), x, y);
			return 0;
		}
	}

	return luaL_error(L, "unknown type '%s'", name);
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

///////////////////////////////////////////////////////////////////////////////
// api

lua_State* script_open(void)
{
	lua_State *L = lua_open();

	//
	// register functions
	//

	luaopen_base(L);

	lua_register(L, "loadmap",  luaT_loadmap);
	lua_register(L, "newmap",   luaT_newmap);
	lua_register(L, "load",     luaT_load);
	lua_register(L, "save",     luaT_save);
	lua_register(L, "import",   luaT_import);
	lua_register(L, "export",   luaT_export);

	lua_register(L, "addbot",   luaT_addbot);
	lua_register(L, "actor",    luaT_actor);

	lua_register(L, "damage",   luaT_damage);

	lua_register(L, "message",  luaT_message);
	lua_register(L, "print",    luaT_print);

	lua_register(L, "quit",     luaT_quit);
	lua_register(L, "pause",    luaT_pause);


	//
	// create global 'gc' table and fill it with editor classes
	//

	lua_newtable(L);
	lua_pushvalue(L, -1);   // make a copy of the table to leave it in the stack
	lua_setglobal(L, "gc"); // set global and pop one element from stack

	for( int i = 0; i < Level::GetTypeCount(); ++i )
	{
		const char *cls = Level::GetTypeName(Level::GetType(i));
		lua_newtable(L);
		lua_setfield(L, -2, cls);
	}

	lua_pop(L, 1); // remove 'gc' table from the stack


	//
	// create global 'classes' table
	//

	lua_newtable(L);
	lua_setglobal(L, "classes"); // set global and pop one element from stack

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
