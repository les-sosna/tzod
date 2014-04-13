// Level.cpp
////////////////////////////////////////////////////////////

#include "Level.h"
#include "Level.inl"
#include "Macros.h"
#include "functions.h"
#include "script.h"
#include "DefaultCamera.h"

#include "core/debug.h"

#include "config/Config.h"
#include "config/Language.h"

//#include "network/TankClient.h"
//#include "network/TankServer.h"

#include "video/RenderBase.h"
#include "video/TextureManager.h" // for ThemeManager

#include "fs/FileSystem.h"
#include "fs/SaveFile.h"
#include "fs/MapFile.h"

#include "ui/GuiManager.h"
#include "ui/gui_desktop.h"
#include "ui/gui.h"
#include "ui/gui_widgets.h"

#include "gc/GameClasses.h"
#include "gc/RigidBodyDinamic.h"
#include "gc/Player.h"
#include "gc/Sound.h"
#include "gc/Camera.h"

//#ifdef _DEBUG
#include "gc/ai.h"
//#endif

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
#include <pluto.h>

#include "ui/ConsoleBuffer.h"
UI::ConsoleBuffer& GetConsole();

#include <GLFW/glfw3.h>


unsigned long FieldCell::_sessionId;

void FieldCell::UpdateProperties()
{
	_prop = 0;
	for( int i = 0; i < _objCount; i++ )
	{
		assert(_ppObjects[i]->GetPassability() > 0);
		if( _ppObjects[i]->GetPassability() > _prop )
			_prop = _ppObjects[i]->GetPassability();
	}
}

void FieldCell::AddObject(GC_RigidBodyStatic *object)
{
	assert(object);
	assert(_objCount < 255);

#ifdef _DEBUG
	for( int i = 0; i < _objCount; ++i )
	{
		assert(object != _ppObjects[i]);
	}
#endif

	GC_RigidBodyStatic **tmp = new GC_RigidBodyStatic* [_objCount + 1];

	if( _ppObjects )
	{
		assert(_objCount > 0);
		memcpy(tmp, _ppObjects, sizeof(GC_RigidBodyStatic*) * _objCount);
		delete[] _ppObjects;
	}

	_ppObjects = tmp;
	_ppObjects[_objCount++] = object;

	UpdateProperties();
}

void FieldCell::RemoveObject(GC_RigidBodyStatic *object)
{
	assert(object);
	assert(_objCount > 0);

	if( 1 == _objCount )
	{
		assert(object == _ppObjects[0]);
		_objCount = 0;
		delete[] _ppObjects;
		_ppObjects = NULL;
	}
	else
	{
		GC_RigidBodyStatic **tmp = new GC_RigidBodyStatic* [_objCount - 1];
		int j = 0;
		for( int i = 0; i < _objCount; ++i )
		{
			if( object == _ppObjects[i] ) continue;
			tmp[j++] = _ppObjects[i];
		}
		_objCount--;
		assert(j == _objCount);
		delete[] _ppObjects;
		_ppObjects = tmp;
	}

	UpdateProperties();
}

////////////////////////////////////////////////////////////

Field::Field()
{
	_cells = NULL;
	_cx    = 0;
	_cy    = 0;

	_edgeCell._prop = 0xFF;
	_edgeCell._x    = -1;
	_edgeCell._y    = -1;
}

Field::~Field()
{
	Clear();
}

void Field::Clear()
{
	if( _cells )
	{
		for( int i = 0; i < _cy; i++ )
			delete[] _cells[i];
		delete[] _cells;
		//-----------
		_cells = NULL;
		_cx    = 0;
		_cy    = 0;
	}
	assert(0 == _cx && 0 == _cy);
}

void Field::Resize(int cx, int cy)
{
	assert(cx > 0 && cy > 0);
	Clear();
	_cx = cx;
	_cy = cy;
	_cells = new FieldCell* [_cy];
	for( int y = 0; y < _cy; y++ )
	{
		_cells[y] = new FieldCell[_cx];
		for( int x = 0; x < _cx; x++ )
		{
			(*this)(x, y)._x = x;
			(*this)(x, y)._y = y;
			if( 0 == x || 0 == y || _cx-1 == x || _cy-1 == y )
				(*this)(x, y)._prop = 0xFF;
		}
	}
	FieldCell::_sessionId = 0;
}

void Field::ProcessObject(GC_RigidBodyStatic *object, bool add)
{
	float r = object->GetRadius() / CELL_SIZE;
	vec2d p = object->GetPos() / CELL_SIZE;

	assert(r >= 0);

	int xmin = std::max(0,       int(p.x - r + 0.5f));
	int xmax = std::min(_cx - 1, int(p.x + r + 0.5f));
	int ymin = std::max(0,       int(p.y - r + 0.5f));
	int ymax = std::min(_cy - 1, int(p.y + r + 0.5f));

	for( int x = xmin; x <= xmax; x++ )
	for( int y = ymin; y <= ymax; y++ )
	{
		if( add )
		{
			(*this)(x, y).AddObject(object);
		}
		else
		{
			(*this)(x, y).RemoveObject(object);
			if( 0 == x || 0 == y || _cx-1 == x || _cy-1 == y )
				(*this)(x, y)._prop = 0xFF;
		}
	}
}

#ifdef _DEBUG
FieldCell& Field::operator() (int x, int y)
{
	assert(NULL != _cells);
	return (x >= 0 && x < _cx && y >= 0 && y < _cy) ? _cells[y][x] : _edgeCell;
}

void Field::Dump()
{
	TRACE("==== Field dump ====");

	for( int y = 0; y < _cy; y++ )
	{
		char buf[1024] = {0};
		for( int x = 0; x < _cx; x++ )
		{
			switch( (*this)(x, y).Properties() )
			{
			case 0:
				strcat(buf, " ");
				break;
			case 1:
				strcat(buf, "-");
				break;
			case 0xFF:
				strcat(buf, "#");
				break;
			}
		}
		TRACE("%s", buf);
	}

	TRACE("=== end of dump ====");
}

#endif

////////////////////////////////////////////////////////////

EditorModeListenerHelper::EditorModeListenerHelper()
{
	g_level->AddEditorModeListener(this);
}

EditorModeListenerHelper::~EditorModeListenerHelper()
{
	g_level->RemoveEditorModeListener(this);
}

////////////////////////////////////////////////////////////

// don't create game objects in the constructor
Level::Level()
  : _modeEditor(false)
#ifdef NETWORK_DEBUG
  , _checksum(0)
  , _frame(0)
  , _dump(NULL)
#endif
  , _serviceListener(NULL)
  , _texBack(g_texman->FindSprite("background"))
  , _texGrid(g_texman->FindSprite("grid"))
  , _frozen(false)
  , _limitHit(false)
  , _sx(0)
  , _sy(0)
  , _locationsX(0)
  , _locationsY(0)
  , _seed(1)
  , _time(0)
  , _safeMode(true)
{
	TRACE("Constructing the level");

	// register config handlers
	g_conf.s_volume.eventChange = std::bind(&Level::OnChangeSoundVolume, this);
	g_conf.sv_nightmode.eventChange = std::bind(&Level::OnChangeNightMode, this);
}

void Level::AddEditorModeListener(IEditorModeListener *ls)
{
	assert(!_editorModeListeners.count(ls));
	_editorModeListeners.insert(ls);
}

void Level::RemoveEditorModeListener(IEditorModeListener *ls)
{
	assert(_editorModeListeners.count(ls));
	_editorModeListeners.erase(ls);
}

bool Level::IsEmpty() const
{
	return GetList(LIST_objects).empty();
}

void Level::Resize(int X, int Y)
{
	assert(IsEmpty());


	//
	// Resize
	//

	_locationsX  = (X * CELL_SIZE / LOCATION_SIZE + ((X * CELL_SIZE) % LOCATION_SIZE != 0 ? 1 : 0));
	_locationsY  = (Y * CELL_SIZE / LOCATION_SIZE + ((Y * CELL_SIZE) % LOCATION_SIZE != 0 ? 1 : 0));
	_sx          = (float) X * CELL_SIZE;
	_sy          = (float) Y * CELL_SIZE;

	for( int i = 0; i < Z_COUNT; i++ )
		z_grids[i].resize(_locationsX, _locationsY);

	grid_rigid_s.resize(_locationsX, _locationsY);
	grid_walls.resize(_locationsX, _locationsY);
	grid_wood.resize(_locationsX, _locationsY);
	grid_water.resize(_locationsX, _locationsY);
	grid_pickup.resize(_locationsX, _locationsY);

	_field.Resize(X + 1, Y + 1);
}

void Level::Clear()
{
	assert(IsSafeMode());
	SetEditorMode(false);

	FOREACH_SAFE(GetList(LIST_objects), GC_Object, obj)
	{
		obj->Kill();
	}

	// reset info
	_infoAuthor.clear();
	_infoEmail.clear();
	_infoUrl.clear();
	_infoDesc.clear();
	_infoTheme.clear();
	_infoOnInit.clear();

	// reset variables
	_time = 0;
	_limitHit = false;
	_frozen = false;
#ifdef NETWORK_DEBUG
	_checksum = 0;
	_frame = 0;
	if( _dump )
	{
		fclose(_dump);
		_dump = NULL;
	}
#endif
}

void Level::HitLimit()
{
	assert(!_limitHit);
//	PauseLocal(true);
	_limitHit = true;
	PLAY(SND_Limit, vec2d(0,0));
}

bool Level::init_emptymap(int X, int Y)
{
	assert(IsSafeMode());

	Clear();
	Resize(X, Y);
	_ThemeManager::Inst().ApplyTheme(0);

	return true;
}

bool Level::init_import_and_edit(const char *mapName)
{
	assert(IsSafeMode());
	assert(IsEmpty());

	g_conf.sv_nightmode.Set(false);

	try
	{
		Import(g_fs->Open(mapName)->QueryStream());
	}
	catch( const std::exception &e )
	{
		GetConsole().WriteLine(1, e.what());
		return false;
	}

	SetEditorMode(true);

	return true;
}

void Level::init_newdm(std::shared_ptr<FS::Stream> s, unsigned long seed)
{
	assert(IsSafeMode());
	assert(IsEmpty());

	_seed       = seed;

	SetEditorMode(false);
	Import(s);

	if( !script_exec(g_env.L, _infoOnInit.c_str()) )
	{
		Clear();
		throw std::runtime_error("init script error");
	}
}

Level::~Level()
{
	assert(IsSafeMode());
	TRACE("Destroying the level");

	// unregister config handlers
	g_conf.s_volume.eventChange = nullptr;
	g_conf.sv_nightmode.eventChange = nullptr;

	assert(IsEmpty() && _garbage.empty());
	assert(!g_env.nNeedCursor);
	assert(_editorModeListeners.empty());
}

void Level::Unserialize(const char *fileName)
{
	assert(IsSafeMode());
	assert(IsEmpty());

	TRACE("Loading saved game from file '%s'...", fileName);

	SetEditorMode(false);

	std::shared_ptr<FS::Stream> stream = g_fs->Open(fileName, FS::ModeRead)->QueryStream();
	SaveFile f(stream, true);

	try
	{
		SaveHeader sh;
		stream->Read(&sh, sizeof(SaveHeader));

		if( VERSION != sh.dwVersion )
			throw std::runtime_error("invalid version");

		g_conf.sv_timelimit.SetFloat(sh.timelimit);
		g_conf.sv_fraglimit.SetInt(sh.fraglimit);
		g_conf.sv_nightmode.Set(sh.nightmode);

		_time = sh.time;
		Resize(sh.width, sh.height);


		// fill pointers cache
		for(;;)
		{
			ObjectType type;
			f.Serialize(type);
			if( INVALID_OBJECT_TYPE == type ) // end of list signal
				break;
			if( GC_Object *obj = RTTypes::Inst().CreateFromFile(type) )
			{
				f.RegPointer(obj);
			}
			else
			{
				TRACE("ERROR: unknown object type - %u", type);
				throw std::runtime_error("Load error: unknown object type");
			}
		}

		// read objects contents in the same order as pointers
		for( ObjectList::iterator it = GetList(LIST_objects).begin(); it != GetList(LIST_objects).end(); ++it )
		{
			(*it)->Serialize(f);
		}


		//
		// restore lua user environment
		//

		struct ReadHelper
		{
			static const char* r(lua_State *L, void* data, size_t *sz)
			{
				static char buf[1];
				try
				{
					reinterpret_cast<FS::Stream*>(data)->Read(buf, sizeof(buf));
					*sz = sizeof(buf);
				}
				catch( const std::exception &e )
				{
					*sz = 0;
					luaL_error(L, "[file read] %s", e.what());
				}
				return buf;
			}
			static int read_user(lua_State *L)
			{
				void *ud = lua_touserdata(L, 1);
				lua_settop(L, 0);
				lua_newtable(g_env.L);       // permanent objects
				lua_pushstring(L, "any_id_12345");
				lua_getfield(L, LUA_REGISTRYINDEX, "restore_ptr");
				lua_settable(L, -3);
				pluto_unpersist(L, &r, ud);
				lua_setglobal(L, "user");    // unpersisted object
				return 0;
			}
			static int read_queue(lua_State *L)
			{
				void *ud = lua_touserdata(L, 1);
				lua_settop(L, 0);
				lua_newtable(g_env.L);       // permanent objects
				pluto_unpersist(L, &r, ud);
				lua_getglobal(L, "pushcmd");
				assert(LUA_TFUNCTION == lua_type(L, -1));
				lua_pushvalue(L, -2);
				lua_setupvalue(L, -2, 1);    // unpersisted object
				return 0;
			}
			static int restore_ptr(lua_State *L)
			{
				assert(1 == lua_gettop(L));
				size_t id = (size_t) lua_touserdata(L, 1);
				SaveFile *f = (SaveFile *) lua_touserdata(L, lua_upvalueindex(1));
				assert(f);
				GC_Object *obj;
				try
				{
					obj = id ? f->RestorePointer(id) : NULL;
				}
				catch( const std::exception &e )
				{
					return luaL_error(L, "%s", e.what());
				}
				luaT_pushobject(L, obj);
				return 1;
			}
		};
		lua_pushlightuserdata(g_env.L, &f);
		lua_pushcclosure(g_env.L, &ReadHelper::restore_ptr, 1);
		lua_setfield(g_env.L, LUA_REGISTRYINDEX, "restore_ptr");
		if( lua_cpcall(g_env.L, &ReadHelper::read_user, stream.get()) )
		{
			std::string err = "[pluto read user] ";
			err += lua_tostring(g_env.L, -1);
			lua_pop(g_env.L, 1);
			throw std::runtime_error(err);
		}
		if( lua_cpcall(g_env.L, &ReadHelper::read_queue, stream.get()) )
		{
			std::string err = "[pluto read queue] ";
			err += lua_tostring(g_env.L, -1);
			lua_pop(g_env.L, 1);
			throw std::runtime_error(err);
		}

		// apply the theme
		_infoTheme = sh.theme;
		_ThemeManager::Inst().ApplyTheme(_ThemeManager::Inst().FindTheme(sh.theme));

		// update skins
		FOREACH( GetList(LIST_players), GC_Player, pPlayer )
		{
			pPlayer->UpdateSkin();
		}

		GC_Camera::UpdateLayout();
	}
	catch( const std::runtime_error& )
	{
		Clear();
		throw;
	}
}

void Level::Serialize(const char *fileName)
{
	assert(IsSafeMode());

	PauseGame(true); // FIXME: exception safety

	TRACE("Saving game to file '%S'...", fileName);

	std::shared_ptr<FS::Stream> stream = g_fs->Open(fileName, FS::ModeWrite)->QueryStream();
	SaveFile f(stream, false);

	SaveHeader sh = {0};
	strcpy(sh.theme, _infoTheme.c_str());
	sh.dwVersion    = VERSION;
	sh.fraglimit    = g_conf.sv_fraglimit.GetInt();
	sh.timelimit    = g_conf.sv_timelimit.GetFloat();
	sh.nightmode    = g_conf.sv_nightmode.Get();
	sh.time         = _time;
	sh.width        = (int) _sx / CELL_SIZE;
	sh.height       = (int) _sy / CELL_SIZE;

	stream->Write(&sh, sizeof(SaveHeader));


	//
	// pointers to game objects
	//

	for( ObjectList::iterator it = GetList(LIST_objects).begin(); it != GetList(LIST_objects).end(); ++it )
	{
		GC_Object *object(*it);
		ObjectType type = object->GetType();
		stream->Write(&type, sizeof(type));
		f.RegPointer(object);
	}
	ObjectType terminator(INVALID_OBJECT_TYPE);
	f.Serialize(terminator);


	//
	// write objects contents in the same order as pointers
	//

	for( ObjectList::iterator it = GetList(LIST_objects).begin(); it != GetList(LIST_objects).end(); ++it )
	{
		(*it)->Serialize(f);
	}


	//
	// write lua user environment
	//

	struct WriteHelper
	{
		static int w(lua_State *L, const void* p, size_t sz, void* ud)
		{
			try
			{
				reinterpret_cast<SaveFile*>(ud)->GetStream()->Write(p, sz);
			}
			catch( const std::exception &e )
			{
				return luaL_error(L, "[file write] %s", e.what());
			}
			return 0;
		}
		static int write_user(lua_State *L)
		{
			void *ud = lua_touserdata(L, 1);
			lua_settop(L, 0);
			lua_newtable(g_env.L);       // permanent objects
			lua_getfield(L, LUA_REGISTRYINDEX, "restore_ptr");
			lua_pushstring(L, "any_id_12345");
			lua_settable(L, -3);
			lua_getglobal(L, "user");    // object to persist
			pluto_persist(L, &w, ud);
			return 0;
		}
		static int write_queue(lua_State *L)
		{
			void *ud = lua_touserdata(L, 1);
			lua_settop(L, 0);
			lua_newtable(g_env.L);       // permanent objects
			lua_getglobal(L, "pushcmd");
			assert(LUA_TFUNCTION == lua_type(L, -1));
			lua_getupvalue(L, -1, 1);    // object to persist
			lua_remove(L, -2);
			pluto_persist(L, &w, ud);
			return 0;
		}
	};
	lua_newuserdata(g_env.L, 0); // placeholder for restore_ptr
	lua_setfield(g_env.L, LUA_REGISTRYINDEX, "restore_ptr");
	if( lua_cpcall(g_env.L, &WriteHelper::write_user, &f) )
	{
		std::string err = "[pluto write user] ";
		err += lua_tostring(g_env.L, -1);
		lua_pop(g_env.L, 1);
		throw std::runtime_error(err);
	}
	if( lua_cpcall(g_env.L, &WriteHelper::write_queue, &f) )
	{
		std::string err = "[pluto write queue] ";
		err += lua_tostring(g_env.L, -1);
		lua_pop(g_env.L, 1);
		throw std::runtime_error(err);
	}
	lua_setfield(g_env.L, LUA_REGISTRYINDEX, "restore_ptr");


	PauseGame(false);
}

void Level::Import(std::shared_ptr<FS::Stream> s)
{
	assert(IsEmpty());
	assert(IsSafeMode());

	MapFile file(s, false);

	int width, height;
	if( !file.getMapAttribute("width", width) ||
		!file.getMapAttribute("height", height) )
	{
		throw std::runtime_error("unknown map size");
	}

	file.getMapAttribute("theme", _infoTheme);
	_ThemeManager::Inst().ApplyTheme(_ThemeManager::Inst().FindTheme(_infoTheme));

	file.getMapAttribute("author",   _infoAuthor);
	file.getMapAttribute("desc",     _infoDesc);
	file.getMapAttribute("link-url", _infoUrl);
	file.getMapAttribute("e-mail",   _infoEmail);
	file.getMapAttribute("on_init",  _infoOnInit);

	Resize(width, height);

	while( file.NextObject() )
	{
		float x = 0;
		float y = 0;
		file.getObjectAttribute("x", x);
		file.getObjectAttribute("y", y);
		ObjectType t = RTTypes::Inst().GetTypeByName(file.GetCurrentClassName());
		if( INVALID_OBJECT_TYPE == t )
			continue;
		GC_Object *object = RTTypes::Inst().GetTypeInfo(t).Create(x, y);
		object->MapExchange(file);
	}
	GC_Camera::UpdateLayout();
}

void Level::Export(std::shared_ptr<FS::Stream> s)
{
	assert(!IsEmpty());
	assert(IsSafeMode());

	MapFile file(s, true);

	//
	// map info
	//

	file.setMapAttribute("type", "deathmatch");

	std::ostringstream str;
	str << VERSION;
	file.setMapAttribute("version", str.str());

	file.setMapAttribute("width",  (int) _sx / CELL_SIZE);
	file.setMapAttribute("height", (int) _sy / CELL_SIZE);

	file.setMapAttribute("author",   _infoAuthor);
	file.setMapAttribute("desc",     _infoDesc);
	file.setMapAttribute("link-url", _infoUrl);
	file.setMapAttribute("e-mail",   _infoEmail);

	file.setMapAttribute("theme",    _infoTheme);

	file.setMapAttribute("on_init",  _infoOnInit);

	// objects
	FOREACH( GetList(LIST_objects), GC_Object, object )
	{
		if( RTTypes::Inst().IsRegistered(object->GetType()) )
		{
			file.BeginObject(RTTypes::Inst().GetTypeName(object->GetType()));
			object->MapExchange(file);
			file.WriteCurrentObject();
		}
	}
}

void Level::PauseSound(bool pause)
{
	FOREACH( GetList(LIST_sounds), GC_Sound, pSound )
	{
		pSound->Freeze(pause);
	}
}

void Level::SetEditorMode(bool editorModeEnable)
{
	if( !_modeEditor ^ !editorModeEnable )
	{
		bool paused = IsGamePaused();
		_modeEditor = editorModeEnable;
		std::for_each(_editorModeListeners.begin(), _editorModeListeners.end(),
			std::bind2nd(std::mem_fun(&IEditorModeListener::OnEditorModeChanged), _modeEditor));
		if( !paused ^ !IsGamePaused() )
		{
			PauseSound(IsGamePaused());
		}
	}
}

GC_2dSprite* Level::PickEdObject(const vec2d &pt, int layer)
{
	for( int i = Z_COUNT; i--; )
	{
		PtrList<ObjectList> receive;
		z_grids[i].OverlapPoint(receive, pt / LOCATION_SIZE);

		PtrList<ObjectList>::iterator rit = receive.begin();
		for( ; rit != receive.end(); rit++ )
		{
			ObjectList::iterator it = (*rit)->begin();
			for( ; it != (*rit)->end(); ++it )
			{
				GC_2dSprite *object = static_cast<GC_2dSprite*>(*it);

				FRECT frect;
				object->GetGlobalRect(frect);

				if( PtInFRect(frect, pt) )
				{
					for( int i = 0; i < RTTypes::Inst().GetTypeCount(); ++i )
					{
						if( object->GetType() == RTTypes::Inst().GetTypeByIndex(i)
						    && (-1 == layer || RTTypes::Inst().GetTypeInfoByIndex(i).layer == layer) )
						{
							return object;
						}
					}
				}
			}
		}
	}

	return NULL;
}

int Level::net_rand()
{
	return ((_seed = _seed * 214013L + 2531011L) >> 16) & RAND_MAX;
}

float Level::net_frand(float max)
{
	return (float) net_rand() / (float) RAND_MAX * max;
}

vec2d Level::net_vrand(float len)
{
	return vec2d(net_frand(PI2)) * len;
}

bool Level::CalcOutstrip( const vec2d &fp, // fire point
                          float vp,        // speed of the projectile
                          const vec2d &tx, // target position
                          const vec2d &tv, // target velocity
                          vec2d &out_fake) // out: fake target position
{
	float vt = tv.len();

	if( vt >= vp || vt < 1e-7 )
	{
		out_fake = tx;
		return false;
	}

	float cg = tv.x / vt;
	float sg = tv.y / vt;

	float x   = (tx.x - fp.x) * cg + (tx.y - fp.y) * sg;
	float y   = (tx.y - fp.y) * cg - (tx.x - fp.x) * sg;
	float tmp = vp*vp - vt*vt;

	float fx = x + vt * (x*vt + sqrt(x*x * vp*vp + y*y * tmp)) / tmp;

	out_fake.x = std::max(0.0f, std::min(_sx, fp.x + fx*cg - y*sg));
	out_fake.y = std::max(0.0f, std::min(_sy, fp.y + fx*sg + y*cg));
	return true;
}

GC_RigidBodyStatic* Level::TraceNearest( Grid<ObjectList> &list,
                                         const GC_RigidBodyStatic* ignore,
                                         const vec2d &x0,      // origin
                                         const vec2d &a,       // direction with length
                                         vec2d *ht,
                                         vec2d *norm) const
{
	DbgLine(x0, x0 + a);

	struct SelectNearest
	{
		const GC_RigidBodyStatic *ignore;
		vec2d x0;
		vec2d lineCenter;
		vec2d lineDirection;

		GC_RigidBodyStatic *result;
		vec2d resultPos;
		vec2d resultNorm;

		SelectNearest()
			: result(NULL)
		{
		}

		bool Select(GC_RigidBodyStatic *obj, vec2d norm, float enter, float exit)
		{
			if( ignore != obj )
			{
				result = obj;
				resultPos = lineCenter + lineDirection * enter;
				resultNorm = norm;

				lineDirection *= enter + 0.5f;
				lineCenter = x0 + lineDirection / 2;
			}
			return false;
		}
		inline const vec2d& GetCenter() const { return lineCenter; }
		inline const vec2d& GetDirection() const { return lineDirection; }
	};
	SelectNearest selector;
	selector.ignore = ignore;
	selector.x0 = x0;
	selector.lineCenter = x0 + a/2;
	selector.lineDirection = a;
	RayTrace(list, selector);
	if( selector.result )
	{
		if( ht ) *ht = selector.resultPos;
		if( norm ) *norm = selector.resultNorm;
	}
	return selector.result;
}

void Level::TraceAll( Grid<ObjectList> &list,
                      const vec2d &x0,      // origin
                      const vec2d &a,       // direction with length
                      std::vector<CollisionPoint> &result) const
{
	struct SelectAll
	{
		vec2d lineCenter;
		vec2d lineDirection;

		std::vector<CollisionPoint> &result;

		explicit SelectAll(std::vector<CollisionPoint> &r)
			: result(r)
		{
		}

		bool Select(GC_RigidBodyStatic *obj, vec2d norm, float enter, float exit)
		{
			CollisionPoint cp;
			cp.obj = obj;
			cp.normal = norm;
			cp.enter = enter;
			cp.exit = exit;
			result.push_back(cp);
			return false;
		}
		inline const vec2d& GetCenter() const { return lineCenter; }
		inline const vec2d& GetDirection() const { return lineDirection; }
	};
	SelectAll selector(result);
	selector.lineCenter = x0 + a/2;
	selector.lineDirection = a;
	RayTrace(list, selector);
}

void Level::DrawBackground(size_t tex) const
{
	const LogicalTexture &lt = g_texman->Get(tex);
	MyVertex *v = g_render->DrawQuad(lt.dev_texture);
	v[0].color = 0xffffffff;
	v[0].u = 0;
	v[0].v = 0;
	v[0].x = 0;
	v[0].y = 0;
	v[1].color = 0xffffffff;
	v[1].u = _sx / lt.pxFrameWidth;
	v[1].v = 0;
	v[1].x = _sx;
	v[1].y = 0;
	v[2].color = 0xffffffff;
	v[2].u = _sx / lt.pxFrameWidth;
	v[2].v = _sy / lt.pxFrameHeight;
	v[2].x = _sx;
	v[2].y = _sy;
	v[3].color = 0xffffffff;
	v[3].u = 0;
	v[3].v = _sy / lt.pxFrameHeight;
	v[3].x = 0;
	v[3].y = _sy;
}

void Level::Step(const std::vector<ControlPacket> &ctrl, float dt)
{
	_time += dt;

	if( !_frozen )
	{
		auto ctrlIt = ctrl.begin();
		FOREACH( GetList(LIST_players), GC_Player, p )
		{
			if( GC_PlayerHuman *ph = dynamic_cast<GC_PlayerHuman *>(p) )
			{
				VehicleState vs;
				ctrlIt->tovs(vs);
				ph->SetControllerState(vs);

				++ctrlIt;
			}
		}
		assert(ctrlIt == ctrl.end());


		_safeMode = false;
		for( ObjectList::safe_iterator it = ts_fixed.safe_begin(); it != ts_fixed.end(); ++it )
		{
			(*it)->TimeStepFixed(dt);
			if( *it )
				(*it)->TimeStepFloat(dt);
		}
		GC_RigidBodyDynamic::ProcessResponse(dt);
		_safeMode = true;
	}

	RunCmdQueue(dt);

	if( g_conf.sv_timelimit.GetInt() && g_conf.sv_timelimit.GetInt() * 60 <= _time )
	{
		HitLimit();
	}

	FOREACH_SAFE( GetList(LIST_sounds), GC_Sound, pSound )
	{
		pSound->KillWhenFinished();
	}


	//
	// sync lost error detection
	//

#ifdef NETWORK_DEBUG
	if( !_dump )
	{
		char fn[MAX_PATH];
		sprintf_s(fn, "network_dump_%u_%u.txt", GetTickCount(), GetCurrentProcessId());
		_dump = fopen(fn, "w");
		assert(_dump);
	}
	++_frame;
	fprintf(_dump, "\n### frame %04d ###\n", _frame);

	DWORD dwCheckSum = 0;
	for( ObjectList::safe_iterator it = ts_fixed.safe_begin(); it != ts_fixed.end(); ++it )
	{
		if( DWORD cs = (*it)->checksum() )
		{
			dwCheckSum = dwCheckSum ^ cs ^ 0xD202EF8D;
			dwCheckSum = (dwCheckSum >> 1) | ((dwCheckSum & 0x00000001) << 31);
			fprintf(_dump, "0x%08x -> local 0x%08x, global 0x%08x  (%s)\n", (*it), cs, dwCheckSum, typeid(**it).name());
		}
	}
	_checksum = dwCheckSum;
	fflush(_dump);
#endif
}

void Level::RunCmdQueue(float dt)
{
	assert(_safeMode);

	lua_State * const L = g_env.L;

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
				GetConsole().WriteLine(1, lua_tostring(g_env.L, -1));
				lua_pop(g_env.L, 1); // pop the error message
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

void Level::Render() const
{
	g_render->SetAmbient(g_conf.sv_nightmode.Get() ? (GetEditorMode() ? 0.5f : 0) : 1);

#ifdef _DEBUG
	FOREACH( GetList(LIST_players), GC_Player, p )
	{
		if( GC_PlayerAI *pp = dynamic_cast<GC_PlayerAI *>(p) )
		{
			pp->debug_draw();
		}
	}
#endif

	if( GetEditorMode() || GetList(LIST_cameras).empty() )
	{
		// render from default camera
		g_render->Camera(NULL, _defaultCamera.GetPosX(), _defaultCamera.GetPosY(), _defaultCamera.GetZoom(), 0);

		FRECT world;
		world.left = _defaultCamera.GetPosX();
		world.top = _defaultCamera.GetPosY();
		world.right = world.left + (float) g_render->GetWidth() / _defaultCamera.GetZoom();
		world.bottom = world.top + (float) g_render->GetHeight() / _defaultCamera.GetZoom();

		RenderInternal(world);
	}
	else
	{
		if( g_render->GetWidth() >= int(_sx) && g_render->GetHeight() >= int(_sy) )
		{
			// render from single camera with maximum shake
			float max_shake = -1;
			GC_Camera *singleCamera = NULL;
			FOREACH( GetList(LIST_cameras), GC_Camera, pCamera )
			{
				if( pCamera->GetShake() > max_shake )
				{
					singleCamera = pCamera;
					max_shake = pCamera->GetShake();
				}
			}
			assert(singleCamera);

			FRECT world;
			singleCamera->GetWorld(world);

			Rect screen;
			singleCamera->GetScreen(screen);

			g_render->Camera(&screen,
				world.left,
				world.top,
				singleCamera->GetZoom(),
				g_conf.g_rotcamera.Get() ? singleCamera->GetAngle() : 0);

			RenderInternal(world);
		}
		else
		{
			// render from each camera
			FOREACH( GetList(LIST_cameras), GC_Camera, pCamera )
			{
				FRECT world;
				pCamera->GetWorld(world);

				Rect screen;
				pCamera->GetScreen(screen);

				g_render->Camera(&screen,
					world.left,
					world.top,
					pCamera->GetZoom(),
					g_conf.g_rotcamera.Get() ? pCamera->GetAngle() : 0);

				RenderInternal(world);
			}
		}
	}

#ifdef _DEBUG
	if (glfwGetKey(g_appWindow, GLFW_KEY_BACKSPACE) != GLFW_PRESS)
#endif
	{
		_dbgLineBuffer.clear();
	}
}

void Level::RenderInternal(const FRECT &world) const
{
	//
	// draw lights to alpha channel
	//

	g_render->SetMode(RM_LIGHT);
	if( g_conf.sv_nightmode.Get() )
	{
		float xmin = std::max(0.0f, world.left);
		float ymin = std::max(0.0f, world.top);
		float xmax = std::min(_sx, world.right);
		float ymax = std::min(_sy, world.bottom);

		FOREACH( GetList(LIST_lights), GC_Light, pLight )
		{
			if( pLight->IsActive() &&
				pLight->GetPos().x + pLight->GetRenderRadius() > xmin &&
				pLight->GetPos().x - pLight->GetRenderRadius() < xmax &&
				pLight->GetPos().y + pLight->GetRenderRadius() > ymin &&
				pLight->GetPos().y - pLight->GetRenderRadius() < ymax )
			{
				pLight->Shine();
			}
		}
	}


	//
	// draw world to rgb
	//

	g_render->SetMode(RM_WORLD);

	// background texture
	DrawBackground(_texBack);
	if( GetEditorMode() && g_conf.ed_drawgrid.Get() )
		DrawBackground(_texGrid);


	int xmin = std::max(0, int(world.left / LOCATION_SIZE));
	int ymin = std::max(0, int(world.top / LOCATION_SIZE));
	int xmax = std::min(_locationsX - 1, int(world.right / LOCATION_SIZE));
	int ymax = std::min(_locationsY - 1, int(world.bottom / LOCATION_SIZE) + 1);

	for( int z = 0; z < Z_COUNT; ++z )
	{
		for( int x = xmin; x <= xmax; ++x )
		for( int y = ymin; y <= ymax; ++y )
		{
			FOREACH(z_grids[z].element(x,y), GC_2dSprite, object)
			{
				object->Draw();
			}
		}

		FOREACH( z_globals[z], GC_2dSprite, object )
		{
			object->Draw();
		}
	}

	if( !_dbgLineBuffer.empty() )
	{
		g_render->DrawLines(&*_dbgLineBuffer.begin(), _dbgLineBuffer.size());
	}
}

#ifndef NDEBUG
void Level::DbgLine(const vec2d &v1, const vec2d &v2, SpriteColor color) const
{
	_dbgLineBuffer.push_back(MyLine());
	MyLine &line = _dbgLineBuffer.back();
	line.begin = v1;
	line.end = v2;
	line.color = color;
}
#endif

bool Level::IsGamePaused() const
{ 
	return g_env.pause > 0
		|| _limitHit 
		|| _modeEditor;
}

GC_Object* Level::FindObject(const std::string &name) const
{
	std::map<std::string, const GC_Object*>::const_iterator it = _nameToObjectMap.find(name);
	return _nameToObjectMap.end() != it ? const_cast<GC_Object*>(it->second) : NULL;
}

void Level::OnChangeSoundVolume()
{
	FOREACH( GetList(LIST_sounds), GC_Sound, pSound )
	{
		pSound->UpdateVolume();
	}
}

void Level::OnChangeNightMode()
{
	FOREACH( GetList(LIST_lights), GC_Light, pLight )
	{
		pLight->Update();
	}
}

///////////////////////////////////////////////////////////////////////////////

PlayerHandle* Level::GetPlayerByIndex(size_t playerIndex)
{
	GC_Player *player = NULL;
	FOREACH(GetList(LIST_players), GC_Player, p)
	{
		if( 0 == playerIndex-- )
		{
			player = p;
			break;
		}
	}
	return player;
}

void Level::SetPlayerInfo(PlayerHandle *p, const PlayerDesc &pd)
{
	GC_Player *player = static_cast<GC_Player *>(p);
	player->SetClass(pd.cls);
	player->SetNick(pd.nick);
	player->SetSkin(pd.skin);
	player->SetTeam(pd.team);
	player->UpdateSkin();
}

void Level::PlayerQuit(PlayerHandle *p)
{
	if( g_gui )
		static_cast<UI::Desktop*>(g_gui->GetDesktop())->GetMsgArea()->WriteLine(g_lang.msg_player_quit.Get());
	static_cast<GC_Player*>(p)->Kill();
}

PlayerHandle* Level::AddHuman(const PlayerDesc &pd)
{
	GC_PlayerLocal *player = new GC_PlayerLocal();
	SetPlayerInfo(player, pd);
	return player;
}

void Level::AddBot(const BotDesc &bd)
{
	GC_PlayerAI *ai = new GC_PlayerAI();
	ai->SetClass(bd.pd.cls);
	ai->SetNick(bd.pd.nick);
	ai->SetSkin(bd.pd.skin);
	ai->SetTeam(bd.pd.team);
	ai->SetLevel(std::max(0U, std::min(AI_MAX_LEVEL, bd.level)));
	ai->UpdateSkin();
}


///////////////////////////////////////////////////////////////////////////////
// end of file
