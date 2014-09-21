// script.cpp

#include "script.h"

#include "BackgroundIntro.h"
#include "gui.h"
#include "gui_desktop.h"
#include "SaveFile.h"
#include "ThemeManager.h"

#include "gc/vehicle.h"
#include "gc/Pickup.h"
#include "gc/Player.h"
#include "gc/Weapons.h" // for ugly workaround
#include "gc/World.h"
#include "gc/WorldEvents.h"
#include "gc/Macros.h"
#include "gclua/lObject.h"
#include "gclua/lWorld.h"

#ifndef NOSOUND
#include "sound/MusicPlayer.h"
#include "sound/sfx.h"
#include "gc/Sound.h"
#endif

#include "core/Debug.h"

//#include "network/TankClient.h"
//#include "network/TankServer.h"

#include <fs/FileSystem.h>
#include <GLFW/glfw3.h>
#include <ui/GuiManager.h>
#include <video/TextureManager.h>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}


///////////////////////////////////////////////////////////////////////////////
// aux

void ClearCommandQueue(lua_State *L)
{
	lua_getglobal(L, "pushcmd");
	 assert(LUA_TFUNCTION == lua_type(L, -1));
	 lua_newtable(L);
	  lua_setupvalue(L, -2, 1); // pops result of lua_newtable
	 lua_pop(L, 1);  // pop result of lua_getglobal
}

void RunCmdQueue(lua_State *L, float dt)
{
	lua_getglobal(L, "pushcmd");
	assert(LUA_TFUNCTION == lua_type(L, -1));
	lua_getupvalue(L, -1, 1);
	int queueidx = lua_gettop(L);
    
	for( lua_pushnil(L); lua_next(L, queueidx); lua_pop(L, 1) )
	{
		// -2 -> key; -1 -> value(table)
        
		lua_rawgeti(L, -1, 2);
		lua_Number time = lua_tonumber(L, -1) - dt;
		lua_pop(L, 1);
        
		if( time <= 0 )
		{
			// call function and remove it from queue
			lua_rawgeti(L, -1, 1);
			if( lua_pcall(L, 0, 0, 0) )
			{
				GetConsole().WriteLine(1, lua_tostring(L, -1));
				lua_pop(L, 1); // pop the error message
			}
			lua_pushvalue(L, -2); // push copy of the key
			lua_pushnil(L);
			lua_settable(L, queueidx);
		}
		else
		{
			// update time value
			lua_pushnumber(L, time);
			lua_rawseti(L, -2, 2);
		}
	}
    
	assert(lua_gettop(L) == queueidx);
	lua_pop(L, 2); // pop results of lua_getglobal and lua_getupvalue
}

///////////////////////////////////////////////////////////////////////////////
// c closures

// exit to the system
static int luaT_quit(lua_State *L)
{
	ScriptEnvironment &se = GetScriptEnvironment(L);
	if( !se.world.IsSafeMode() )
		return luaL_error(L, "attempt to execute 'quit' in unsafe mode");
	se.exitCommand();
	return 0;
}

static int luaT_game(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
		return luaL_error(L, "wrong number of arguments: 1 expected, got %d", n);

    World &world = GetScriptEnvironment(L).world;
	if( !world.IsSafeMode() )
		return luaL_error(L, "attempt to execute 'game' in unsafe mode");

	const char *clientType = luaL_checkstring(L, 1);

	// TODO: delete client after all checks
//	SAFE_DELETE(g_client); // it will clear level, message area, command queue

	if( *clientType )
	{
		if( !strcmp("intro", clientType) )
		{
//			new IntroClient(&world);
		}
		else
		{
			return luaL_error(L, "unknown client type: %s", clientType);
		}
	}

	return 0;
}

// start/stop the timer
static int luaT_pause(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
		return luaL_error(L, "wrong number of arguments: 1 expected, got %d", n);

	luaL_checktype(L, 1, LUA_TBOOLEAN);

//	PauseGame( 0 != lua_toboolean(L, 1) );
    GetConsole().WriteLine(0, "pause - function is unavailable in this version");

	return 0;
}

static int luaT_freeze(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
		return luaL_error(L, "wrong number of arguments: 1 expected, got %d", n);

	luaL_checktype(L, 1, LUA_TBOOLEAN);

//	world.Freeze( 0 != lua_toboolean(L, 1) );
	GetConsole().WriteLine(0, "freeze - function is unavailable in this version");

	return 0;
}

// loadmap( string filename )
static int luaT_loadmap(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
		return luaL_error(L, "wrong number of arguments: 1 or 2 expected, got %d", n);

	const char *filename = luaL_checkstring(L, 1);
	ScriptEnvironment &se = GetScriptEnvironment(L);
	if( !se.world.IsSafeMode() )
		return luaL_error(L, "attempt to execute 'loadmap' in unsafe mode");

	try
	{
        se.world.Clear();
        se.world.Seed(rand());
        se.world.Import(se.fs.Open(filename)->QueryStream(), se.themeManager, se.textureManager);
        
        if( !script_exec(L, se.world._infoOnInit.c_str()) )
        {
            se.world.Clear();
            throw std::runtime_error("init script error");
        }
	}
	catch( const std::exception &e )
	{
		return luaL_error(L, "couldn't load map '%s' - %s", filename, e.what());
	}

	return 0;
}

// newmap(int x_size, int y_size)
static int luaT_newmap(lua_State *L)
{
	int n = lua_gettop(L);
	if( 2 != n )
		return luaL_error(L, "wrong number of arguments: 2 expected, got %d", n);

	int x = std::max(LEVEL_MINSIZE, std::min(LEVEL_MAXSIZE, luaL_checkint(L, 1) ));
	int y = std::max(LEVEL_MINSIZE, std::min(LEVEL_MAXSIZE, luaL_checkint(L, 2) ));
	ScriptEnvironment &se = GetScriptEnvironment(L);
	if( !se.world.IsSafeMode() )
		return luaL_error(L, "attempt to execute 'newmap' in unsafe mode");
    
	se.world.Clear();
	se.world.Resize(x, y);
	se.themeManager.ApplyTheme(0, se.textureManager);

	return 0;
}

// load( string filename )  -- load a saved game
static int luaT_load(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
		return luaL_error(L, "wrong number of arguments: 1 expected, got %d", n);

	const char *filename = luaL_checkstring(L, 1);
    ScriptEnvironment &se = GetScriptEnvironment(L);
	if( !se.world.IsSafeMode() )
		return luaL_error(L, "attempt to execute 'load' in unsafe mode");

//	SAFE_DELETE(g_client);

	try
	{
		TRACE("Loading saved game from file '%s'...", filename);
		std::shared_ptr<FS::Stream> stream = se.fs.Open(filename, FS::ModeRead)->QueryStream();
		se.world.Unserialize(stream, se.themeManager, se.textureManager);
	}
	catch( const std::exception &e )
	{
		return luaL_error(L, "couldn't load game from '%s' - %s", filename, e.what());
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

    ScriptEnvironment &se = GetScriptEnvironment(L);
	if( !se.world.IsSafeMode() )
		return luaL_error(L, "attempt to execute 'save' in unsafe mode");

	se.world.PauseSound(true);
	try
	{
		TRACE("Saving game to file '%S'...", filename);
		std::shared_ptr<FS::Stream> stream = se.fs.Open(filename, FS::ModeWrite)->QueryStream();
		se.world.Serialize(stream);
	}
	catch( const std::exception &e )
	{
		se.world.PauseSound(false);
		return luaL_error(L, "couldn't save game to '%s' - %s", filename, e.what());
	}
	se.world.PauseSound(false);
	GetConsole().Printf(0, "game saved: '%s'", filename);
	return 0;
}

// import( string filename )  -- import map
static int luaT_import(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
		return luaL_error(L, "wrong number of arguments: 1 expected, got %d", n);

	const char *filename = luaL_checkstring(L, 1);

    ScriptEnvironment &se = GetScriptEnvironment(L);
	if( !se.world.IsSafeMode() )
		return luaL_error(L, "attempt to execute 'import' in unsafe mode");

//	SAFE_DELETE(g_client);
    
	try
	{
        se.world.Clear();
		se.world.Import(se.fs.Open(filename)->QueryStream(), se.themeManager, se.textureManager);
        g_conf.sv_nightmode.Set(false);
	}
	catch( const std::exception &e )
	{
		GetConsole().WriteLine(1, e.what());
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

    ScriptEnvironment &se = GetScriptEnvironment(L);
	if( !se.world.IsSafeMode() )
		return luaL_error(L, "attempt to execute 'export' in unsafe mode");

	try
	{
		se.world.Export(se.fs.Open(filename, FS::ModeWrite)->QueryStream());
	}
	catch( const std::exception &e )
	{
		return luaL_error(L, "couldn't export map to '%s' - %s", filename, e.what());
	}

	GetConsole().Printf(0, "map exported: '%s'", filename);

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
    
    ScriptEnvironment &se = GetScriptEnvironment(L);
    if (se.world._messageListener)
        se.world._messageListener->OnGameMessage(buf.str().c_str());

	return 0;
}

// select a soundtraack
static int luaT_music(lua_State *L)
{
	int n = lua_gettop(L);     // get number of arguments
	if( 1 != n )
		return luaL_error(L, "wrong number of arguments: 1 expected, got %d", n);

#ifndef NOSOUND
	const char *filename = luaL_checkstring(L, 1);
	ScriptEnvironment &se = GetScriptEnvironment(L);
	if( filename[0] )
	{
		try
		{
			se.music.reset(new MusicPlayer());
			if( se.music->Load(se.fs.GetFileSystem(DIR_MUSIC)->Open(filename)->QueryMap()) )
			{
				se.music->Play();
				lua_pushboolean(L, true);
				return 1;
			}
			else
			{
				TRACE("WARNING: Could not load music file '%s'. Unsupported format?", filename);
			}
		}
		catch( const std::exception &e )
		{
			TRACE("WARNING: Could not load music file '%s' - %s", filename, e.what())
		}
	}
	se.music.reset();
#endif

	lua_pushboolean(L, false);
	return 1;
}

static int luaT_print(lua_State *L)
{
	std::stringstream buf;
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
		if( i > 1 ) buf << " "; // delimiter
		buf << s;
		lua_pop(L, 1);         // pop result
	}
	GetConsole().WriteLine(0, buf.str());
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

#if 0
int luaT_PlaySound(lua_State *L)
{
	int n = lua_gettop(L);     // get number of arguments
	if( 2 != n )
		return luaL_error(L, "wrong number of arguments: 2 expected, got %d", n);

	GC_Actor *actor = luaT_checkobjectT<GC_Actor>(L, 1);
	const char *filename = luaL_checkstring(L, 2);

	if( filename[0] )
	{
		LoadOggVorbis(true, SND_User1, filename);
		PLAY(SND_User1,actor->GetPos());
	}
	return 0;
}
#endif


int luaT_loadtheme(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
	{
		return luaL_error(L, "1 argument expected; got %d", n);
	}

	const char *filename = luaL_checkstring(L, 1);
	ScriptEnvironment &se = GetScriptEnvironment(L);
	try
	{
		if( 0 == se.textureManager.LoadPackage(filename, se.fs.Open(filename)->QueryMap(), se.fs) )
		{
			GetConsole().WriteLine(1, "WARNING: there are no textures loaded");
		}
	}
	catch( const std::exception &e )
	{
		return luaL_error(L, "could not load theme - %s", e.what());
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

lua_State* script_open(ScriptEnvironment &se)
{
	lua_State *L = luaL_newstate();
    if (!L)
        throw std::bad_alloc();

	//
	// open libs
	//

	static const luaL_Reg lualibs[] = {
		// standard Lua libs
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
		
		// game libs
		{"object", luaopen_object},
		{"world", luaopen_world},
		
		{NULL, NULL}
	};

	for( const luaL_Reg *lib = lualibs; lib->func; ++lib )
	{
		lua_pushcfunction(L, lib->func);
		lua_pushstring(L, lib->name);
		lua_call(L, 1, 0);
	}
    
    
    //
    // set script environment
    //
    
    lua_pushlightuserdata(L, &se);
    lua_setfield(L, LUA_REGISTRYINDEX, "ENVIRONMENT");


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
	lua_register(L, "music",    luaT_music);
	lua_register(L, "message",  luaT_message);
	lua_register(L, "print",    luaT_print);
	lua_register(L, "game",     luaT_game);
	lua_register(L, "quit",     luaT_quit);
	lua_register(L, "pause",    luaT_pause);
	lua_register(L, "freeze",   luaT_freeze);
//	lua_register(L, "play_sound",   luaT_PlaySound);

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
	for( int i = 0; i < RTTypes::Inst().GetTypeCount(); ++i )
	{
		lua_newtable(L);
		lua_setfield(L, -2, RTTypes::Inst().GetTypeInfoByIndex(i).name);
	}
	lua_setglobal(L, "gc"); // set global and pop one element from stack


	lua_newtable(L);
	lua_setglobal(L, "classes");

	lua_newtable(L);
	lua_setglobal(L, "user");
	
	return L;
}

void script_close(lua_State *L)
{
	assert(L);
	lua_close(L);
}

bool script_exec(lua_State *L, const char *string)
{
	assert(L);

	if( luaL_loadstring(L, string) )
	{
		GetConsole().Printf(1, "syntax error %s", lua_tostring(L, -1));
		lua_pop(L, 1); // pop the error message from the stack
		return false;
	}

	if( lua_pcall(L, 0, 0, 0) )
	{
		GetConsole().WriteLine(1, lua_tostring(L, -1));
		lua_pop(L, 1); // pop the error message from the stack
		return false;
	}

	return true;
}

bool script_exec_file(lua_State *L, const char *filename)
{
	assert(L);

	try
	{
		ScriptEnvironment &se = GetScriptEnvironment(L); // fixme: may throw lua_error
		std::shared_ptr<FS::MemMap> f = se.fs.Open(filename)->QueryMap();
		if( luaL_loadbuffer(L, f->GetData(), f->GetSize(), filename) )
		{
			std::string msg(lua_tostring(L, -1));
			lua_pop(L, 1); // pop error message
			throw std::runtime_error(msg);
		}
		if( lua_pcall(L, 0, 0, 0) )
		{
			std::string err = lua_tostring(L, -1);
			lua_pop(L, 1); // pop the error message from the stack
			throw std::runtime_error(std::string("runtime error: ") + err);
		}
	}
	catch( const std::runtime_error &e )
	{
		GetConsole().WriteLine(1, e.what());
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// end of file
