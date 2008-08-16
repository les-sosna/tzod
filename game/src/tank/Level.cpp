// Level.cpp
////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "pluto.h"


#include "Level.h"
#include "macros.h"
#include "functions.h"

#include "core/debug.h"
#include "core/Console.h"

#include "config/Config.h"

#include "network/TankClient.h"
#include "network/TankServer.h"

#include "video/RenderBase.h"
#include "video/TextureManager.h" // for ThemeManager

#include "fs/SaveFile.h"
#include "fs/MapFile.h"

#include "ui/GuiManager.h"
#include "ui/gui_desktop.h"
#include "ui/gui.h"

#include "gc/GameClasses.h"
#include "gc/RigidBodyDinamic.h"
#include "gc/Player.h"
#include "gc/Sound.h"
#include "gc/Camera.h"

//#ifdef _DEBUG
#include "gc/ai.h"
//#endif


////////////////////////////////////////////////////////////
unsigned long FieldCell::_sessionId;

void FieldCell::UpdateProperties()
{
	_prop = 0;
	for( int i = 0; i < _objCount; i++ )
	{
		_ASSERT(_ppObjects[i]->GetPassability() > 0);
		if( _ppObjects[i]->GetPassability() > _prop )
			_prop = _ppObjects[i]->GetPassability();
	}
}

void FieldCell::AddObject(GC_RigidBodyStatic *object)
{
	_ASSERT(object);
	_ASSERT(!object->IsKilled());
	_ASSERT(_objCount < 255);

#ifdef _DEBUG
	for( int i = 0; i < _objCount; ++i )
	{
		_ASSERT(object != _ppObjects[i]);
	}
#endif

	GC_RigidBodyStatic **tmp = new GC_RigidBodyStatic* [_objCount + 1];

	if( _ppObjects )
	{
		_ASSERT(_objCount > 0);
		memcpy(tmp, _ppObjects, sizeof(GC_RigidBodyStatic*) * _objCount);
		delete[] _ppObjects;
	}

	_ppObjects = tmp;
	_ppObjects[_objCount++] = object;

	UpdateProperties();
}

void FieldCell::RemoveObject(GC_RigidBodyStatic *object)
{
	_ASSERT(object);
	_ASSERT(_objCount > 0);

	if( 1 == _objCount )
	{
		_ASSERT(object == _ppObjects[0]);
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
		_ASSERT(j == _objCount);
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
	_ASSERT(0 == _cx && 0 == _cy);
}

void Field::Resize(int cx, int cy)
{
	_ASSERT(cx > 0 && cy > 0);
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

	_ASSERT(r > 0);

	int xmin = __max(0,       int(p.x - r + 0.5f));
	int xmax = __min(_cx - 1, int(p.x + r + 0.5f));
	int ymin = __max(0,       int(p.y - r + 0.5f));
	int ymax = __min(_cy - 1, int(p.y + r + 0.5f));

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
	_ASSERT(NULL != _cells);
	return (x >= 0 && x < _cx && y >= 0 && y < _cy) ? _cells[y][x] : _edgeCell;
}

void Field::Dump()
{
	TRACE("==== Field dump ====\n");

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
		TRACE("%s\n", buf);
	}

	TRACE("=== end of dump ====\n");
}

#endif

////////////////////////////////////////////////////////////

// в конструкторе нельз€ создавать игровые объекты
Level::Level()
  : _modeEditor(false)
  , _pause(0)
{
	TRACE("Constructing the level\n");

	_time        = 0;
	_timeBuffer  = 0;
	_limitHit    = false;
	_freezed     = false;

	_safeMode    = true;

	_locations_x  = 0;
	_locations_y  = 0;
	_sx = 0;
	_sy = 0;

	_seed   = 1;

	_gameType = -1;

	/////////////////////////
	#ifdef _DEBUG
	_bInitialized = FALSE;
	#endif
	#ifdef NETWORK_DEBUG
	_checksum = 0;
	#endif


	// register config handlers
	g_conf->s_volume->eventChange.bind(&Level::OnChangeSoundVolume, this);
	g_conf->sv_nightmode->eventChange.bind(&Level::OnChangeNightMode, this);
}

void Level::Init(int X, int Y)
{
	Resize(X, Y);

	//
	// other objects
	//

	new GC_Camera((GC_Player *) NULL);

	_background = new GC_Background();

	_temporaryText = new GC_Text(0, 0, "");
	_temporaryText->Show(false);
}

void Level::Resize(int X, int Y)
{
	_locations_x  = X * CELL_SIZE / LOCATION_SIZE + ((X * CELL_SIZE) % LOCATION_SIZE != 0 ? 1 : 0);
	_locations_y  = Y * CELL_SIZE / LOCATION_SIZE + ((Y * CELL_SIZE) % LOCATION_SIZE != 0 ? 1 : 0);
	_sx           = (float) X * CELL_SIZE;
	_sy           = (float) Y * CELL_SIZE;

	for( int i = 0; i < Z_COUNT; i++ )
		z_grids[i].resize(_locations_x, _locations_y);

	grid_rigid_s.resize(_locations_x, _locations_y);
	grid_walls.resize(_locations_x, _locations_y);
	grid_wood.resize(_locations_x, _locations_y);
	grid_water.resize(_locations_x, _locations_y);
	grid_pickup.resize(_locations_x, _locations_y);

	_field.Resize(X + 1, Y + 1);
}

void Level::HitLimit()
{
	_ASSERT(!_limitHit);
	PauseLocal(true);
	_limitHit = true;
	PLAY(SND_Limit, vec2d(0,0));
}

bool Level::init_emptymap()
{
	_ASSERT(!_bInitialized);
	_ASSERT(_bInitialized = TRUE);

	_gameType   = GT_EDITOR;

	_ThemeManager::Inst().ApplyTheme(0);

	ToggleEditorMode();
	_ASSERT(_modeEditor);

	g_conf->sv_nightmode->Set(false);

	return true;
}

bool Level::init_import_and_edit(const char *mapName)
{
	_ASSERT(!_bInitialized);
	_ASSERT(_bInitialized = TRUE);

	_gameType = GT_EDITOR;
	g_conf->sv_nightmode->Set(false);

	if( !Import(mapName, false) )
		return false;

	ToggleEditorMode();
	_ASSERT(_modeEditor);

	return true;
}

bool Level::init_newdm(const char *mapName, unsigned long seed)
{
	_ASSERT(!_bInitialized);
	_ASSERT(_bInitialized = TRUE);

	_gameType   = GT_DEATHMATCH;
	_modeEditor = false;
	_seed       = seed;

	return Import(mapName, true);
}

bool Level::init_load(const char *fileName)
{
	_ASSERT(!_bInitialized);
	_modeEditor = false;
	return Unserialize(fileName);
}

Level::~Level()
{
	_ASSERT(IsSafeMode());
	TRACE("Destroying the level\n");

	static_cast<UI::Desktop*>(g_gui->GetDesktop())->ShowEditor(false);


	SAFE_KILL(_temporaryText);
	SAFE_KILL(_background);

	OBJECT_LIST::safe_iterator it = GetList(LIST_objects).safe_begin();
	while( GetList(LIST_objects).end() != it )
	{
		GC_Object* obj = *it;
		_ASSERT(!obj->IsKilled());
		obj->Kill();
		++it;
	}

	// unregister config handlers
	g_conf->s_volume->eventChange.clear();
	g_conf->sv_nightmode->eventChange.clear();

	//-------------------------------------------
	_ASSERT(!g_env.nNeedCursor);
	_ASSERT(GetList(LIST_objects).empty());
}

bool Level::Unserialize(const char *fileName)
{
	_ASSERT(!_bInitialized);
	_ASSERT(_bInitialized = TRUE);
	_ASSERT(IsSafeMode());

	TRACE("Loading saved game from file '%s'\n", fileName);

	SaveFile f;
	f._load = true;
	f._file = CreateFile(fileName,
	                     GENERIC_READ,
	                     0,
	                     NULL,
	                     OPEN_EXISTING,
	                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
	                     NULL);

	if( INVALID_HANDLE_VALUE == f._file )
	{
		TRACE("ERROR: couldn't open file\n");
		return false;
	}

	bool result = true;
	try
	{
		DWORD bytesRead = 0;

		SaveHeader sh = {0};
		ReadFile(f._file, &sh, sizeof(SaveHeader), &bytesRead, NULL);
		if( sizeof(SaveHeader) != bytesRead )
			throw "ERROR: unexpected end of file";

		if( VERSION != sh.dwVersion )
			throw "ERROR: invalid version";


		//
		// restoring lua user environment
		//
		struct ReadHelper
		{
			static const char* r(lua_State *L, void* data, size_t *sz)
			{
				static char buf[1];
				DWORD bytesRead = 0;
				ReadFile((HANDLE) data, buf, sizeof(buf), &bytesRead, NULL);
				*sz = bytesRead;
				return bytesRead ? buf : NULL;
			}
			static int read_user(lua_State *L)
			{
				void *ud = lua_touserdata(L, 1);
				lua_settop(L, 0);
				lua_newtable(g_env.L);       // permanent objects
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
				_ASSERT(LUA_TFUNCTION == lua_type(L, -1));
				lua_pushvalue(L, -2);
				lua_setupvalue(L, -2, 1);    // unpersisted object
				return 0;
			}
		};
		if( lua_cpcall(g_env.L, &ReadHelper::read_user, f._file) )
		{
			const char *err = lua_tostring(g_env.L, -1);
			TRACE("%s\n", err);
			lua_pop(g_env.L, 1);
			throw "ERROR: pluto user";
		}
		if( lua_cpcall(g_env.L, &ReadHelper::read_queue, f._file) )
		{
			const char *err = lua_tostring(g_env.L, -1);
			TRACE("%s\n", err);
			lua_pop(g_env.L, 1);
			throw "ERROR: pluto queue";
		}


		_gameType = sh.dwGameType;

		g_conf->sv_timelimit->SetFloat(sh.timelimit);
		g_conf->sv_fraglimit->SetInt(sh.fraglimit);
		g_conf->sv_nightmode->Set(sh.nightmode);

		_time = sh.time;
		Init(sh.width, sh.height);

		while( sh.nObjects > 0 )
		{
			GC_Object::CreateFromFile(f);
			sh.nObjects--;
		}

		// restore links
		if( !f.RestoreAllLinks() )
			throw "ERROR: invalid links";

		// apply the theme
		_infoTheme = sh.theme;
		_ThemeManager::Inst().ApplyTheme(_ThemeManager::Inst().FindTheme(sh.theme));

		// update skins
		FOREACH( GetList(LIST_players), GC_Player, pPlayer )
		{
			pPlayer->UpdateSkin();
		}

		GC_Camera::SwitchEditor();
	}
	catch (const char *msg)
	{
		TRACE("%s\n", msg);
		result = false;
	}

	CloseHandle(f._file);
	return result;
}

bool Level::Serialize(const char *fileName)
{
	_ASSERT(IsSafeMode());

	TRACE("Saving game to file '%s'\n", fileName);

	DWORD bytesWritten = 0;

	SaveFile f;
	f._load = false;
	f._file = CreateFile(fileName,
	                     GENERIC_WRITE,
	                     0,
	                     NULL,
	                     CREATE_ALWAYS,
	                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
	                     NULL);
	if( INVALID_HANDLE_VALUE == f._file )
	{
		TRACE("ERROR: couldn't open file for writing\n");
		return false;
	}

	bool result = true;
    try
	{
		SaveHeader sh = {0};
		strcpy(sh.theme, _infoTheme.c_str());
		sh.dwVersion    = VERSION;
		sh.dwGameType   = _gameType;
		sh.fraglimit    = g_conf->sv_fraglimit->GetInt();
		sh.timelimit    = g_conf->sv_timelimit->GetFloat();
		sh.nightmode    = g_conf->sv_nightmode->Get();
		sh.time         = _time;
		sh.width        = (int) _sx / CELL_SIZE;
		sh.height       = (int) _sy / CELL_SIZE;
		sh.nObjects     = 0; // будем увеличивать по мере записи

		WriteFile(f._file, &sh, sizeof(SaveHeader), &bytesWritten, NULL);
		if( bytesWritten != sizeof(SaveHeader) )
			throw "ERROR: couldn't write file. check disk space";


		//
		// writing lua user environment
		//
		struct WriteHelper
		{
			static int w(lua_State *L, const void* p, size_t sz, void* ud)
			{
				DWORD written = 0;
				WriteFile((HANDLE) ud, p, sz, &written, NULL);
				return sz - written;
			}
			static int write_user(lua_State *L)
			{
				void *ud = lua_touserdata(L, 1);
				lua_settop(L, 0);
				lua_newtable(g_env.L);       // permanent objects
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
				_ASSERT(LUA_TFUNCTION == lua_type(L, -1));
				lua_getupvalue(L, -1, 1);    // object to persist
				lua_remove(L, -2);
				pluto_persist(L, &w, ud);
				return 0;
			}
		};
		if( lua_cpcall(g_env.L, &WriteHelper::write_user, f._file) )
		{
			const char *err = lua_tostring(g_env.L, -1);
			TRACE("%s\n", err);
			lua_pop(g_env.L, 1);
			throw "ERROR: pluto user";
		}
		if( lua_cpcall(g_env.L, &WriteHelper::write_queue, f._file) )
		{
			const char *err = lua_tostring(g_env.L, -1);
			TRACE("%s\n", err);
			lua_pop(g_env.L, 1);
			throw "ERROR: pluto queue";
		}


		//перебираем все объекты. если нужно - сохран€ем
		OBJECT_LIST::reverse_iterator it = GetList(LIST_objects).rbegin();
		for( ; it != GetList(LIST_objects).rend(); ++it )
		{
			GC_Object *object = *it;
			if( object->IsSaved() )
			{
				ObjectType type = object->GetType();
				WriteFile(f._file, &type, sizeof(type), &bytesWritten, NULL);
				try
				{
					SafePtr<void> tmp;
					SetRawPtr(tmp, object);
					f.Serialize(tmp);
					SetRawPtr(tmp, NULL);

					object->Serialize(f);
				}
				catch (...)
				{
					throw "ERROR: serialize object failed\n";
				}
				sh.nObjects++;
			}
		}

		// return to begin of file and write count of saved objects
		SetFilePointer(f._file, 0, NULL, FILE_BEGIN);
		WriteFile(f._file, &sh, sizeof(SaveHeader), &bytesWritten, NULL);
		if( bytesWritten != sizeof(SaveHeader) )
			throw "ERROR: couldn't write file. check disk space";
	}
	catch(const char *msg)
	{
		TRACE("%s\n", msg);
		result = false;
	}
	CloseHandle(f._file);

	return result;
}

bool Level::Import(const char *fileName, bool execInitScript)
{
	_ASSERT(IsSafeMode());

	MapFile file;
	if( !file.Open(fileName, false) )
		return false;

	int width, height;
	if( !file.getMapAttribute("width", width) ||
		!file.getMapAttribute("height", height) )
	{
		return false;
	}

	file.getMapAttribute("theme", _infoTheme);
	_ThemeManager::Inst().ApplyTheme(_ThemeManager::Inst().FindTheme(_infoTheme.c_str()));

	file.getMapAttribute("author",   _infoAuthor);
	file.getMapAttribute("desc",     _infoDesc);
	file.getMapAttribute("link-url", _infoUrl);
	file.getMapAttribute("e-mail",   _infoEmail);
	file.getMapAttribute("on_init",  _infoOnInit);

	Init(width, height);

	while( file.NextObject() )
	{
		float x, y;
		if( !file.getObjectAttribute("x", x) ) continue;
		if( !file.getObjectAttribute("y", y) ) continue;
		name2type::iterator it = get_n2t().find(file.GetCurrentClassName());
		if( get_n2t().end() == it ) continue;
		GC_Object *object = get_t2i()[it->second].Create(x, y);
		object->mapExchange(file);
	}
	GC_Camera::SwitchEditor();
	return !execInitScript || script_exec(g_env.L, _infoOnInit.c_str());
}

bool Level::Export(const char *fileName)
{
	_ASSERT(IsSafeMode());

	MapFile file;

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

	//
	// objects
	//

	if( !file.Open(fileName, true) )
		return false;

	FOREACH( GetList(LIST_objects), GC_Object, object )
	{
		if( object->IsKilled() ) continue;
		if( IsRegistered(object->GetType()) )
		{
			file.BeginObject(GetTypeName(object->GetType()));
			object->mapExchange(file);
			if( !file.WriteCurrentObject() ) return false;
		}
	}

	return true;
}

void Level::PauseLocal(bool pause)
{
	if( pause )
	{
		if( 0 == _pause + g_env.pause )
		{
			PauseSound(true);
		}
		++_pause;
	}
	else
	{
		_ASSERT(_pause + g_env.pause > 0);
		--_pause;
		if( 0 == _pause + g_env.pause )
		{
			PauseSound(false);
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

void Level::ToggleEditorMode()
{
	if( GT_INTRO == _gameType )
	{
		return;
	}

	if( _modeEditor )
	{
		_modeEditor = false;
		_background->EnableGrid(false);
		PauseLocal(false);
	}
	else
	{
		_modeEditor = true;
		_background->EnableGrid(g_conf->ed_drawgrid->Get());
		PauseLocal(true);
	}
	GC_Camera::SwitchEditor();
}

GC_Object* Level::CreateObject(ObjectType type, float x, float y)
{
	_ASSERT(IsRegistered(type));
	return get_t2i()[type].Create(x, y);
}

GC_2dSprite* Level::PickEdObject(const vec2d &pt, int layer)
{
	for( int i = Z_COUNT; i--; )
	{
		std::vector<OBJECT_LIST*> receive;
		z_grids[i].OverlapCircle(receive, pt.x / LOCATION_SIZE, pt.y / LOCATION_SIZE, 0);

		std::vector<OBJECT_LIST*>::iterator rit = receive.begin();
		for( ; rit != receive.end(); rit++ )
		{
			OBJECT_LIST::iterator it = (*rit)->begin();
			for( ; it != (*rit)->end(); ++it )
			{
				GC_2dSprite *object = static_cast<GC_2dSprite*>(*it);

				FRECT frect;
				object->GetGlobalRect(frect);

				if( PtInFRect(frect, pt) )
				{
					for( int i = 0; i < GetTypeCount(); ++i )
					{
						if( object->GetType() == GetTypeByIndex(i)
						    && (-1 == layer || GetTypeInfoByIndex(i).layer == layer) )
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

void Level::CalcOutstrip( const vec2d &fp, // fire point
                          float vp,        // speed of the projectile
                          const vec2d &tx, // target position
                          const vec2d &tv, // target velocity
                          vec2d &out_fake) // out: fake target position
{
	float vt = tv.len();

	if( vt >= vp || vt < 1e-7 )
	{
		out_fake = tx;
	}
	else
	{
		float cg = tv.x / vt;
		float sg = tv.y / vt;

		float x   = (tx.x - fp.x) * cg + (tx.y - fp.y) * sg;
		float y   = (tx.y - fp.y) * cg - (tx.x - fp.x) * sg;
		float tmp = vp*vp - vt*vt;

		float fx = x + vt * (x*vt + sqrtf(x*x * vp*vp + y*y * tmp)) / tmp;

		out_fake.x = __max(0, __min(_sx, fp.x + fx*cg - y*sg));
		out_fake.y = __max(0, __min(_sy, fp.y + fx*sg + y*cg));
	}
}

void Level::LocationFromPoint(const vec2d &pt, Location &l)
{
	int x = __max(0, int((pt.x + LOCATION_SIZE / 4) / (LOCATION_SIZE / 2)) - 1);
	int y = __max(0, int((pt.y + LOCATION_SIZE / 4) / (LOCATION_SIZE / 2)) - 1);

	l.level = (x & 1) + ((y & 1) << 1);
	l.x = __min(_locations_x - 1, x >> 1);
	l.y = __min(_locations_y - 1, y >> 1);
}

GC_RigidBodyStatic* Level::agTrace( GridSet<OBJECT_LIST> &list,
	                                const GC_RigidBodyStatic* ignore,
	                                const vec2d &x0,  // origin
	                                const vec2d &a,   // direction
	                                vec2d *ht,
	                                vec2d *norm ) const
{
	vec2d hit;
	float nx, ny;

	GC_RigidBodyStatic *pBestObject = NULL;
	float minLen = 1.0e8;


	//
	// overlap line
	//

	vec2d x0_tmp(x0), a_tmp(a);
	x0_tmp /= LOCATION_SIZE;
	a_tmp /= LOCATION_SIZE;

	static const float dx[4] = {0,-0.5f, 0,-0.5f};
	static const float dy[4] = {0, 0,-0.5f,-0.5f};

	const int stepx[2] = {a_tmp.x >= 0 ? 1 : -1, 0};
	const int stepy[2] = {0, a_tmp.y >= 0 ? 1 : -1};
	const int check[2] = {a_tmp.y == 0,   a_tmp.x == 0};

	float la = a.len();
	float la_tmp = la / LOCATION_SIZE;

	for( int i = 0; i < 4; i++ )
	{
		float cx = x0_tmp.x + dx[i];
		float cy = x0_tmp.y + dy[i];

		int targx = (int) floor(cx + a_tmp.x);
		int targy = (int) floor(cy + a_tmp.y);
		int curx  = (int) floor(cx);
		int cury  = (int) floor(cy);

		int xmin = __min(curx, targx);
		int xmax = __max(curx, targx);
		int ymin = __min(cury, targy);
		int ymax = __max(cury, targy);

		for(;;)
		{
			if( curx >= 0 && curx < _locations_x &&
				cury >= 0 && cury < _locations_y )
			{
				OBJECT_LIST &tmp_list = list(i, curx, cury);
				for( OBJECT_LIST::iterator it = tmp_list.begin(); it != tmp_list.end(); ++it )
				{
					GC_RigidBodyStatic *object = (GC_RigidBodyStatic *) *it;
					if( object->GetTrace0() || ignore == object || object->IsKilled() )
					{
						continue;
					}

					//грубо
					bool bHasHit = true;
					// FIXME! TODO: estimate


					// точно
					if( bHasHit )
					{
						vec2d m[4];

						for( int i = 0; i < 4; ++i )
						{
							m[i] = object->GetVertex(i);
						}

						for( int n = 0; n < 4; ++n )
						{
							float xb = m[n].x;
							float yb = m[n].y;

							float bx = m[(n+1)&3].x - xb;
							float by = m[(n+1)&3].y - yb;

							float delta = a.y*bx - a.x*by;
							if( delta <= 0 ) continue;

							float tb = (a.x*(yb - x0.y) - a.y*(xb - x0.x)) / delta;

							if( tb <= 1 && tb >= 0 )
							{
								float len = (bx*(yb - x0.y) - by*(xb - x0.x)) / delta;

								if( len >= 0 && len < minLen )
								{
									minLen = len;
									pBestObject = object;
									nx =  by;
									ny = -bx;
									hit.Set(xb + bx * tb, yb + by * tb);
								}
							}
						}
					}
				}
			}

			if( curx == targx && cury == targy ) break;

			float d_min;
			int   j_min = 0;
			for( int j = 0; j < 2; j++ )
			{
				int newx = curx + stepx[j];
				int newy = cury + stepy[j];

				if( newx < xmin || newx > xmax || newy < ymin || newy > ymax )
				{
					j_min = 1 - j;
					break;
				}

				float d = fabsf(a_tmp.x*((float) newy + 0.5f - cy) -
					a_tmp.y*((float) newx + 0.5f - cx)) / la_tmp;

				if( 0 == j )
					d_min = d;
				else if( d < d_min || check[j] )
				{
					j_min = j;
					d_min = d;
				}
			}
			curx += stepx[j_min];
			cury += stepy[j_min];
		}// for(;;)
	}

	if( pBestObject )
	{
		if( ( ((x0.x <= hit.x) && (hit.x <= x0.x + a.x))
		      ||((x0.x + a.x <= hit.x) && (hit.x <= x0.x)) ) &&
		    ( ((x0.y <= hit.y) && (hit.y <= x0.y + a.y))
		      ||((x0.y + a.y <= hit.y) && (hit.y <= x0.y)) ) )
		{
			if( norm )
			{
				float l = sqrtf(nx*nx + ny*ny);
				norm->Set(nx / l, ny / l);
			}
			if( ht ) *ht = hit;
			return pBestObject;
		}
	}

	return NULL;
}

void Level::DrawText(const char *string, const vec2d &position, enumAlignText align)
{
	_temporaryText->MoveTo(position);
	_temporaryText->SetText(string);
	_temporaryText->SetAlign(align);
	_temporaryText->Draw();
}

void Level::TimeStep(float dt)
{
	if( g_env.pause + _pause > 0 && !g_client && _gameType != GT_INTRO || _limitHit )
		return;

	_dbgLineBuffer.clear();


	dt *= g_conf->sv_speed->GetFloat() / 100.0f;
	_ASSERT(dt >= 0);

	_ASSERT(!_modeEditor);


	int    passedFixedLoopsCount = 0;
	float  passedFixedDT;

	_safeMode = false;

	if( g_client )
	{
		//
		// network mode
		//

		const float fixed_dt = 1.0f / g_conf->sv_fps->GetFloat() / NET_MULTIPLER;

		_timeBuffer += dt;
		while( _timeBuffer >= 0 )
		{
			//
			// обработка команд кадра
			//
			DataBlock db;
			while( g_client->IsGameStarted() && g_client->GetData(db) )
			{
				switch( db.type() )
				{
					case DBTYPE_TEXTMESSAGE:
						static_cast<UI::Desktop*>(g_gui->GetDesktop())->GetMsgArea()->puts((const char*) db.data());
						break;
					case DBTYPE_SERVERQUIT:
						static_cast<UI::Desktop*>(g_gui->GetDesktop())->GetMsgArea()->puts("—ервер вышел");
						break;
					case DBTYPE_PLAYERQUIT:
					{
						const DWORD &id = db.cast<DWORD>();
						OBJECT_LIST::iterator it = GetList(LIST_players).begin();
						while( it != GetList(LIST_players).end() )
						{
							if( GC_PlayerRemote *p = dynamic_cast<GC_PlayerRemote*>(*it) )
							{
								if( p->GetNetworkID() == id )
								{
									static_cast<UI::Desktop*>(g_gui->GetDesktop())->GetMsgArea()->puts("игрок вышел");
									p->Kill();
									break;
								}
							}
							++it;
						}
					} break;
					case DBTYPE_CONTROLPACKET:
					{
						_ASSERT(0 == db.size() % sizeof(ControlPacket));
						size_t count = db.size() / sizeof(ControlPacket);
						for( size_t i = 0; i < count; i++ )
							g_client->_ctrlBuf.push( ((ControlPacket *) db.data())[i] );
					} break;
				} // end of switch( db.type )

				if( DBTYPE_CONTROLPACKET == db.type() ) break; // split frames
			}// end of while g_client->GetData

			if( g_client->_ctrlBuf.empty() )
			{
				_timeBuffer = 0;
				break; // нет кадра. пропускаем
			}


			//
			// ок, кадр получен. расчет игровой ситуации
			//

			#ifdef NETWORK_DEBUG
			DWORD dwCheckSum = 0, tmp_cs;
			#endif


			_time       += fixed_dt;
			_timeBuffer -= fixed_dt;

			if( !_freezed )
			{
				OBJECT_LIST::safe_iterator it = ts_fixed.safe_begin();
				while( it != ts_fixed.end() )
				{
					GC_Object* pTS_Obj = *it;
					_ASSERT(!pTS_Obj->IsKilled());

					// расчет контрольной суммы дл€ обнаружени€ потери синхронизации
					#ifdef NETWORK_DEBUG
					if( tmp_cs = pTS_Obj->checksum() )
					{
						dwCheckSum = dwCheckSum ^ tmp_cs ^ 0xD202EF8D;
						dwCheckSum = (dwCheckSum >> 1) | ((dwCheckSum & 0x00000001) << 31);
					}
					#endif

					pTS_Obj->TimeStepFixed(fixed_dt);
					++it;
				}
				GC_RigidBodyDynamic::ProcessResponse(fixed_dt);
			}
			passedFixedLoopsCount++;
			passedFixedDT = fixed_dt;

#ifdef NETWORK_DEBUG
			_checksum = dwCheckSum;
#endif

			NetworkStats ns;
			g_client->GetStatistics(&ns);

			if( ns.nFramesInBuffer < g_conf->sv_latency->GetInt() )
			{
				break;
			}
		} // end of while( _timeBuffer > 0 )
	}
	else // if( g_client )
	{
		int count = int(dt / MAX_DT_FIXED) + 1;
		const float fixed_dt = dt / (float) count;

		do
		{
			_time += fixed_dt;
			if( !_freezed )
			{
				OBJECT_LIST::safe_iterator it = ts_fixed.safe_begin();
				while( it != ts_fixed.end() )
				{
					GC_Object* pTS_Obj = *it;
					_ASSERT(!pTS_Obj->IsKilled());
					pTS_Obj->TimeStepFixed(fixed_dt);
					++it;
				}
				GC_RigidBodyDynamic::ProcessResponse(fixed_dt);
			}
			passedFixedLoopsCount++;
			passedFixedDT = fixed_dt;
		}
		while( --count );
	} // end of if( g_client )

	if( !_freezed )
	{
		OBJECT_LIST::safe_iterator it = ts_floating.safe_begin();
		while( it != ts_floating.end() )
		{
			GC_Object* pTS_Obj = *it;
			_ASSERT(!pTS_Obj->IsKilled());
			pTS_Obj->TimeStepFloat(dt);
			++it;
		}
	}

	_safeMode = true;


	//
	// run through the command queue
	//

	lua_State * const L = g_env.L;

	lua_getglobal(L, "pushcmd");
	_ASSERT(LUA_TFUNCTION == lua_type(L, -1));
	lua_getupvalue(L, -1, 1);
	int queueidx = lua_gettop(L);

	while( passedFixedLoopsCount-- )
	{
		for( lua_pushnil(L); lua_next(L, queueidx); lua_pop(L, 1) )
		{
			// -2 -> key; -1 -> value(table)

			lua_rawgeti(L, -1, 2);
			lua_Number time = lua_tonumber(L, -1) - passedFixedDT;
			lua_pop(L, 1);

			if( time <= 0 )
			{
				// call function and remove from queue
				lua_rawgeti(L, -1, 1);
				if( lua_pcall(L, 0, 0, 0) )
				{
					TRACE("%s\n", lua_tostring(g_env.L, -1));
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
	}

	_ASSERT(lua_gettop(L) == queueidx);
	lua_pop(L, 2); // pop results of lua_getglobal and lua_getupvalue

#if 0
#ifdef _DEBUG
	// check for dead objects
	OBJECT_LIST::safe_iterator it = GetList(LIST_objects).safe_begin();
	while( GetList(LIST_objects).end() != it )
	{
		_ASSERT(!(*it)->IsKilled());
		++it;
	}
#endif
#endif

	if( g_conf->sv_timelimit->GetInt() && g_conf->sv_timelimit->GetInt() * 60 <= _time )
	{
		HitLimit();
	}
}


void Level::Render() const
{
	//
	// определение активных камер и выбор конфигурации диспле€
	//

	GC_Camera *pMaxShake = NULL;

	if( g_render->GetWidth() >= (int) _sx &&
		g_render->GetHeight() >= (int) _sy )
	{
		float max_shake = 0;
		FOREACH( GetList(LIST_cameras), GC_Camera, pCamera )
		{
			if( !pCamera->IsActive() ) continue;
			if( pCamera->GetShake() >= max_shake )
			{
				pMaxShake = pCamera;
				max_shake = pCamera->GetShake();
			}
		}
	}

	g_render->SetAmbient( g_conf->sv_nightmode->Get() ? 0.0f : 1.0f );

	int count = 0;
	FOREACH( GetList(LIST_cameras), GC_Camera, pCamera )
	{
		if( pMaxShake ) pCamera = pMaxShake;
		if( !pCamera->IsActive() ) continue;


		//
		// рендеринг освещени€
		//

		g_render->SetMode(RM_LIGHT);
		pCamera->Select();
		if( g_conf->sv_nightmode->Get() )
		{
			float xmin = (float) __max(0, g_env.camera_x );
			float ymin = (float) __max(0, g_env.camera_y );
			float xmax = __min(_sx, (float) g_env.camera_x +
				(float) g_render->GetViewportWidth() / pCamera->_zoom );
			float ymax = __min(_sy, (float) g_env.camera_y +
				(float) g_render->GetViewportHeight() / pCamera->_zoom );

			FOREACH( GetList(LIST_lights), GC_Light, pLight )
			{
				_ASSERT(!pLight->IsKilled());
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
		// paint all objects
		//

		g_render->SetMode(RM_WORLD);

		// paint background texture
		_background->Draw();

		for( int z = 0; z < Z_COUNT; ++z )
		{
			// loop over grid sets
			for( int lev = 0; lev < 4; ++lev )
			{
				static const int dx[] = {0, LOCATION_SIZE/2, 0, LOCATION_SIZE/2};
				static const int dy[] = {0, 0, LOCATION_SIZE/2, LOCATION_SIZE/2};

				int xmin = __max(0, (g_env.camera_x - dx[lev]) / LOCATION_SIZE);
				int ymin = __max(0, (g_env.camera_y - dy[lev]) / LOCATION_SIZE);
				int xmax = __min(_locations_x - 1,
					((g_env.camera_x + int((float) g_render->GetViewportWidth() /
					pCamera->_zoom)) - dx[lev]) / LOCATION_SIZE);
				int ymax = __min(_locations_y - 1,
					((g_env.camera_y + int((float) g_render->GetViewportHeight() /
					pCamera->_zoom)) - dy[lev]) / LOCATION_SIZE);

				for( int x = xmin; x <= xmax; ++x )
				for( int y = ymin; y <= ymax; ++y )
				{
					// FIXME: using of global g_level
					FOREACH( g_level->z_grids[z](lev).element(x,y), GC_2dSprite, object )
					{
						_ASSERT(!object->IsKilled());
						object->Draw();
						_ASSERT(!object->IsKilled());
					}
				}
			} // loop over grid sets


			// loop over globals
			// FIXME: using of global g_level
			FOREACH( g_level->z_globals[z], GC_2dSprite, object )
			{
				_ASSERT(!object->IsKilled());
				object->Draw();
				_ASSERT(!object->IsKilled());
			}
		}

		if( pMaxShake ) break;
	} // cameras

#ifdef _DEBUG
	FOREACH( g_level->GetList(LIST_players), GC_Player, p )
	{
		if( GC_PlayerAI *pp = dynamic_cast<GC_PlayerAI *>(p) )
		{
			pp->debug_draw();
		}
	}
#endif

	if( !_dbgLineBuffer.empty() )
	{
		g_render->DrawLines(&*_dbgLineBuffer.begin(), _dbgLineBuffer.size());
	}
}

void Level::DbgLine(const vec2d &v1, const vec2d &v2, SpriteColor color)
{
	_dbgLineBuffer.push_back(MyLine());
	MyLine &line = _dbgLineBuffer.back();
	line.begin = v1;
	line.end = v2;
	line.color = color;
}

GC_Object* Level::FindObject(const char *name) const
{
	std::map<string_t, const GC_Object*>::const_iterator it = _nameToObjectMap.find(name);
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
		_ASSERT(!pLight->IsKilled());
		pLight->Update();
	}
}


///////////////////////////////////////////////////////////////////////////////
// end of file
