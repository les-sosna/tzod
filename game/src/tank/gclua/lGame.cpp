#include "lGame.h"
#include "lgcmod.h"

#include "constants.h"
#include "GameEvents.h"
#include "ThemeManager.h"
#include "config/Config.h"
#include "gc/World.h"
#include "gc/WorldEvents.h"
#include "core/Debug.h"
#include <fs/FileSystem.h>
#include <video/TextureManager.h>

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
}


// exit to the system
static int game_quit(lua_State *L)
{
	ScriptEnvironment &se = GetScriptEnvironment(L);
	if( !se.world.IsSafeMode() )
		return luaL_error(L, "attempt to execute 'quit' in unsafe mode");
	se.exitCommand();
	return 0;
}

static int game_start(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
		return luaL_error(L, "wrong number of arguments: 1 expected, got %d", n);

    World &world = GetScriptEnvironment(L).world;
	if( !world.IsSafeMode() )
		return luaL_error(L, "attempt to execute 'game' in unsafe mode");

	const char *clientType = luaL_checkstring(L, 1);

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
static int game_pause(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
		return luaL_error(L, "wrong number of arguments: 1 expected, got %d", n);

	luaL_checktype(L, 1, LUA_TBOOLEAN);

//	PauseGame( 0 != lua_toboolean(L, 1) );
    GetConsole().WriteLine(0, "pause - function is unavailable in this version");

	return 0;
}

static int game_freeze(lua_State *L)
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
static int game_loadmap(lua_State *L)
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
	}
	catch( const std::exception &e )
	{
		return luaL_error(L, "couldn't load map '%s' - %s", filename, e.what());
	}

	return 0;
}

// newmap(int x_size, int y_size)
static int game_newmap(lua_State *L)
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
static int game_load(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
		return luaL_error(L, "wrong number of arguments: 1 expected, got %d", n);

	const char *filename = luaL_checkstring(L, 1);
    ScriptEnvironment &se = GetScriptEnvironment(L);
	if( !se.world.IsSafeMode() )
		return luaL_error(L, "attempt to execute 'load' in unsafe mode");

	try
	{
		TRACE("Loading saved game from file '%s'...", filename);
		std::shared_ptr<FS::Stream> stream = se.fs.Open(filename, FS::ModeRead)->QueryStream();
		
		// TODO: do full game context serialization
		se.world.Unserialize(stream, se.themeManager, se.textureManager);
	}
	catch( const std::exception &e )
	{
		return luaL_error(L, "couldn't load game from '%s' - %s", filename, e.what());
	}

	return 0;
}

// save( string filename )  -- save game
static int game_save(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
		return luaL_error(L, "wrong number of arguments: 1 expected, got %d", n);

	const char *filename = luaL_checkstring(L, 1);

    ScriptEnvironment &se = GetScriptEnvironment(L);
	if( !se.world.IsSafeMode() )
		return luaL_error(L, "attempt to execute 'save' in unsafe mode");

	try
	{
		TRACE("Saving game to file '%S'...", filename);
		std::shared_ptr<FS::Stream> stream = se.fs.Open(filename, FS::ModeWrite)->QueryStream();

		// TODO: do full game context serialization
		se.world.Serialize(stream);
	}
	catch( const std::exception &e )
	{
		return luaL_error(L, "couldn't save game to '%s' - %s", filename, e.what());
	}
	GetConsole().Printf(0, "game saved: '%s'", filename);
	return 0;
}

// savemap( string filename )  -- save map
static int game_savemap(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
		return luaL_error(L, "wrong number of arguments: 1 expected, got %d", n);

	const char *filename = luaL_checkstring(L, 1);

    ScriptEnvironment &se = GetScriptEnvironment(L);
	if( !se.world.IsSafeMode() )
		return luaL_error(L, "attempt to execute 'savemap' in unsafe mode");

	try
	{
		se.world.Export(se.fs.Open(filename, FS::ModeWrite)->QueryStream());
	}
	catch( const std::exception &e )
	{
		return luaL_error(L, "couldn't save map to '%s' - %s", filename, e.what());
	}

	GetConsole().Printf(0, "map saved: '%s'", filename);

	return 0;
}

// print a message to the MessageArea
static int game_message(lua_State *L)
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
	se.gameListener.OnGameMessage(buf.str().c_str());

	return 0;
}

// select a soundtraack
static int game_music(lua_State *L)
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

static int game_loadtheme(lua_State *L)
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

static const luaL_Reg gamelib[] = {
	{"quit", game_quit},
	{"start", game_start},
	{"pause", game_pause},
	{"freeze", game_freeze},
	{"loadmap", game_loadmap},
	{"savemap", game_savemap},
	{"newmap", game_newmap},
	{"load", game_load},
	{"save", game_save},
	{"message", game_message},
	{"music", game_music},
	{"loadtheme", game_loadtheme},
	{nullptr, nullptr}
};

int luaopen_game(lua_State *L)
{
	luaL_register(L, "game", gamelib);
	return 1;
}
