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

#include "fs/FileSystem.h"
#include "fs/SaveFile.h"

#include "sound/MusicPlayer.h"

#include "core/debug.h"

#include "video/TextureManager.h"

#include "network/TankClient.h"
#include "network/TankServer.h"

#include "functions.h"

///////////////////////////////////////////////////////////////////////////////
// aux

void luaT_pushobject(lua_State *L, GC_Object *obj)
{
	GC_Object **ppObj = (GC_Object **) lua_newuserdata(L, sizeof(obj));
	luaL_getmetatable(L, "object");
	lua_setmetatable(L, -2);
	*ppObj = obj;
	if( *ppObj )
		(*ppObj)->AddRef();
}

///////////////////////////////////////////////////////////////////////////////
// helper functions

static int luaT_objfinalizer(lua_State *L)
{
	assert(1 == lua_gettop(L));
	GC_Object **ppObj = (GC_Object **) lua_touserdata(L, 1);
	if( *ppObj )
		(*ppObj)->Release();
	return 0;
}

static int luaT_objpersist(lua_State *L)
{
	assert(3 == lua_gettop(L));
	GC_Object **ppObj = (GC_Object **) lua_touserdata(L, 1);
	if( *ppObj && (*ppObj)->IsKilled() )
	{
		(*ppObj)->Release();
		(*ppObj) = NULL;
	}
	SaveFile *f = (SaveFile *) lua_touserdata(L, 3);
	assert(f);

	if( luaL_loadstring(L,
		"return function(restore_ptr, id) return function() return restore_ptr(id) end end") )
	{
		return lua_error(L); // use error message on stack
	}
	lua_call(L, 0, 1);

	lua_getfield(L, LUA_REGISTRYINDEX, "restore_ptr");
	lua_pushlightuserdata(L, (void *) (*ppObj ? f->GetPointerId(*ppObj) : 0));
	lua_call(L, 2, 1);
	return 1;
}

static GC_Object* luaT_checkobject(lua_State *L, int n) throw()
{
	//
	// resolve by reference
	//

	if( GC_Object **ppObj = (GC_Object **) lua_touserdata(L, n) )
	{
		if( lua_getmetatable(L, n) )
		{
			luaL_getmetatable(L, "object");
			if( !lua_rawequal(L, -1, -2) )
			{
				luaL_argerror(L, n, "not an object reference");
			}
			lua_pop(L, 2); // pop both tables

			if( !*ppObj || (*ppObj)->IsKilled() )
			{
				luaL_argerror(L, n, "reference to dead object");
			}
			else
			{
				return *ppObj;
			}
		}
	}


	//
	// resolve by name
	//

	const char *name = lua_tostring(L, n);
	if( !name )
	{
		luaL_typerror(L, n, "object or name");
	}

	GC_Object *obj = g_level->FindObject(name);
	if( !obj || obj->IsKilled() )
	{
		luaL_error(L, "object with name '%s' does not exist", name);
	}

	return obj;
}

template<class T>
static T* luaT_checkobjectT(lua_State *L, int n) throw()
{
	GC_Object *obj = luaT_checkobject(L, n);
	T *result = dynamic_cast<T *>(obj);
	if( !result )
	{
		luaL_argerror(L, n, "incompatible object type");
	}
	return result;
}

///////////////////////////////////////////////////////////////////////////////
// c closures

// exit to the system
static int luaT_quit(lua_State *L)
{
	if( !g_level->IsSafeMode() )
		return luaL_error(L, "attempt to execute 'quit' in unsafe mode");
	DestroyWindow(g_env.hMainWnd);
	return 0;
}

static int luaT_reset(lua_State *L)
{
	if( !g_level->IsSafeMode() )
		return luaL_error(L, "attempt to execute 'reset' in unsafe mode");

	g_level->Clear();
	SAFE_DELETE(g_client);
	SAFE_DELETE(g_server);


	//
	// clear the queue
	//

	lua_getglobal(L, "pushcmd");
	assert(LUA_TFUNCTION == lua_type(L, -1));
	lua_newtable(L);
	lua_setupvalue(L, -2, 1); // pops result of lua_newtable
	lua_pop(L, 1);  // pop result of lua_getglobal


	//
	// clear message area
	//

	static_cast<UI::Desktop*>(g_gui->GetDesktop())->GetMsgArea()->Clear();

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

//	g_level->Freeze( 0 != lua_toboolean(L, 1) );
	GetConsole().WriteLine(0, "freeze - function is unavailable in this version");

	return 0;
}

// loadmap( string filename )
static int luaT_loadmap(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
		return luaL_error(L, "wrong number of arguments: 1 expected, got %d", n);

	const char *filename = luaL_checkstring(L, 1);

	if( !g_level->IsSafeMode() )
		return luaL_error(L, "attempt to execute 'loadmap' in unsafe mode");

	g_level->Clear();

	try
	{
		g_level->init_newdm(g_fs->Open(filename)->QueryStream(), rand());
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

	int x = __max(LEVEL_MINSIZE, __min(LEVEL_MAXSIZE, luaL_checkint(L, 1) ));
	int y = __max(LEVEL_MINSIZE, __min(LEVEL_MAXSIZE, luaL_checkint(L, 2) ));

	if( !g_level->IsSafeMode() )
		return luaL_error(L, "attempt to execute 'newmap' in unsafe mode");

	g_level->Clear();
	if( !g_level->init_emptymap(x, y) )
	{
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

	if( !g_level->IsSafeMode() )
		return luaL_error(L, "attempt to execute 'load' in unsafe mode");

	SAFE_DELETE(g_client);
	SAFE_DELETE(g_server);
	g_level->Clear();

	try
	{
		g_level->Unserialize(filename);
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

	if( !g_level->IsSafeMode() )
		return luaL_error(L, "attempt to execute 'save' in unsafe mode");

	g_level->PauseSound(true);
	try
	{
		g_level->Serialize(filename);
	}
	catch( const std::exception &e )
	{
		g_level->PauseSound(false);
		return luaL_error(L, "couldn't save game to '%s' - %s", filename, e.what());
	}
	g_level->PauseSound(false);
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

	if( !g_level->IsSafeMode() )
		return luaL_error(L, "attempt to execute 'import' in unsafe mode");

	SAFE_DELETE(g_client);
	SAFE_DELETE(g_server);
	g_level->Clear();

	if( !g_level->init_import_and_edit(filename) )
	{
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

	if( !g_level->IsSafeMode() )
		return luaL_error(L, "attempt to execute 'export' in unsafe mode");

	try
	{
		g_level->Export(g_fs->Open(filename, FS::ModeWrite)->QueryStream());
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
	static_cast<UI::Desktop*>(g_gui->GetDesktop())->GetMsgArea()->WriteLine(buf.str());
	return 0;
}

// select a soundtraack
static int luaT_music(lua_State *L)
{
	int n = lua_gettop(L);     // get number of arguments
	if( 1 != n )
		return luaL_error(L, "wrong number of arguments: 1 expected, got %d", n);

	const char *filename = luaL_checkstring(L, 1);

	if( filename[0] )
	{
		try
		{
			g_music = SafePtr<MusicPlayer>(new MusicPlayer());
			if( g_music->Load(g_fs->GetFileSystem(DIR_MUSIC)->Open(filename)->QueryMap()) )
			{
				g_music->Play(true);
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

	g_music = NULL;
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
		assert(FALSE);
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
		assert(properties);

		for( lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1) )
		{
			// now 'key' is at index -2 and 'value' at index -1
			pset_helper(properties, L);
		}

		properties->Exchange(true);
	}

	luaT_pushobject(L, obj);
	return 1;
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
		assert(properties);

		for( lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1) )
		{
			// now 'key' is at index -2 and 'value' at index -1
			pset_helper(properties, L);
		}

		properties->Exchange(true);
	}

	luaT_pushobject(L, obj);
	return 1;
}

// object("object name")
int luaT_object(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
	{
		return luaL_error(L, "1 argument expected; got %d", n);
	}

	GC_Object *obj = luaT_checkobject(L, 1);
	luaT_pushobject(L, obj);
	return 1;
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
	GC_RigidBodyStatic *rbs = luaT_checkobjectT<GC_RigidBodyStatic>(L, 2);

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

	luaT_checkobject(L, 1)->Kill();
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
	lua_pushboolean(L, NULL != g_level->FindObject(name));
	return 1;
}


// local x,y = position("object name")
int luaT_position(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
	{
		return luaL_error(L, "1 argument expected; got %d", n);
	}

	GC_Actor *actor = luaT_checkobjectT<GC_Actor>(L, 1);
	lua_pushnumber(L, actor->GetPos().x);
	lua_pushnumber(L, actor->GetPos().y);

	return 2;
}


// type("object name")
int luaT_objtype(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
	{
		return luaL_error(L, "1 argument expected; got %d", n);
	}

	GC_Object *obj = luaT_checkobject(L, 1);
	lua_pushstring(L, Level::GetTypeName(obj->GetType()));
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

	GC_Object *obj = luaT_checkobject(L, 1);
	const char *prop = luaL_checkstring(L, 2);

	SafePtr<PropertySet> properties = obj->GetProperties();
	assert(properties);

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
				assert(FALSE);
			}
			return 1;
		}
	}

	return luaL_error(L, "object of type '%s' has no property '%s'", 
		Level::GetTypeName(obj->GetType()), prop);
}


// pset("object name", "property name", value)
int luaT_pset(lua_State *L)
{
	int n = lua_gettop(L);
	if( 3 != n )
	{
		return luaL_error(L, "3 arguments expected; got %d", n);
	}

	GC_Object *obj = luaT_checkobject(L, 1);
	const char *prop = luaL_checkstring(L, 2);
	luaL_checkany(L, 3);  // prop value should be here

	SafePtr<PropertySet> properties = obj->GetProperties();
	assert(properties);

	// prop name at -2; prop value at -1
	if( !pset_helper(properties, L) )
	{
		return luaL_error(L, "object of type '%s' has no property '%s'", 
			Level::GetTypeName(obj->GetType()), prop);
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

	GC_Vehicle *target = luaT_checkobjectT<GC_Vehicle>(L, 1);
	GC_Pickup *pickup = luaT_checkobjectT<GC_Pickup>(L, 2);

	if( pickup->GetCarrier() && pickup->GetCarrier() != target )
	{
		pickup->Detach();
		pickup->Attach(target);
	}

	return 0;
}

// ai_attack(player, x, y)
int luaT_ai_march(lua_State *L)
{
	int n = lua_gettop(L);
	if( 3 != n )
	{
		return luaL_error(L, "3 arguments expected; got %d", n);
	}

	GC_PlayerAI *who = luaT_checkobjectT<GC_PlayerAI>(L, 1);
	float x = (float) luaL_checknumber(L, 2);
	float y = (float) luaL_checknumber(L, 3);

	lua_pushboolean(L, who->March(x, y));
	return 1;
}

// ai_attack(player, target)
int luaT_ai_attack(lua_State *L)
{
	int n = lua_gettop(L);
	if( 2 != n )
	{
		return luaL_error(L, "2 argument expected; got %d", n);
	}

	GC_PlayerAI *who = luaT_checkobjectT<GC_PlayerAI>(L, 1);
	GC_RigidBodyStatic *what = luaT_checkobjectT<GC_RigidBodyStatic>(L, 2);

	lua_pushboolean(L, who->Attack(what));
	return 1;
}

// ai_pickup(player, target)
int luaT_ai_pickup(lua_State *L)
{
	int n = lua_gettop(L);
	if( 2 != n )
	{
		return luaL_error(L, "2 argument expected; got %d", n);
	}

	GC_PlayerAI *who = luaT_checkobjectT<GC_PlayerAI>(L, 1);
	GC_Pickup *what = luaT_checkobjectT<GC_Pickup>(L, 2);

	lua_pushboolean(L, who->Pickup(what));
	return 1;
}

int luaT_loadtheme(lua_State *L)
{
	int n = lua_gettop(L);
	if( 1 != n )
	{
		return luaL_error(L, "1 argument expected; got %d", n);
	}

	const char *filename = luaL_checkstring(L, 1);

	try
	{
		if( 0 == g_texman->LoadPackage(filename, g_fs->Open(filename)->QueryMap()) )
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
	// init metatable for object variables
	//

	luaL_newmetatable(L, "object");
	 lua_pushstring(L, "__gc");
	  lua_pushcfunction(L, luaT_objfinalizer);
	   lua_settable(L, -3);
	 lua_pushstring(L, "__persist");
	  lua_pushcfunction(L, luaT_objpersist);
	   lua_settable(L, -3);
	 lua_pop(L, 1); // pop the metatable


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

	lua_register(L, "object",   luaT_object);

	lua_register(L, "actor",    luaT_actor);
	lua_register(L, "service",  luaT_service);

	lua_register(L, "damage",   luaT_damage);
	lua_register(L, "kill",     luaT_kill);
	lua_register(L, "pget",     luaT_pget);
	lua_register(L, "pset",     luaT_pset);
	lua_register(L, "equip",    luaT_equip);
	lua_register(L, "exists",   luaT_exists);
	lua_register(L, "position", luaT_position);
	lua_register(L, "objtype",  luaT_objtype);

	lua_register(L, "ai_march", luaT_ai_march);
	lua_register(L, "ai_attack",luaT_ai_attack);
	lua_register(L, "ai_pickup",luaT_ai_pickup);

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
		SafePtr<FS::MemMap> f = g_fs->Open(filename)->QueryMap();
		if( luaL_loadbuffer(L, f->GetData(), f->GetSize(), filename) )
		{
			throw std::runtime_error(lua_tostring(L, -1));
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
