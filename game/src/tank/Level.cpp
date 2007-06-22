// Level.cpp
////////////////////////////////////////////////////////////

#include "stdafx.h"
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

#include "gc/GameClasses.h"
#include "gc/RigidBody.h"
#include "gc/Player.h"
#include "gc/Sound.h"
#include "gc/Camera.h"


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
	_ASSERT(_objCount < 255);

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

void Field::ProcessObject(GC_RigidBodyStatic *object, bool oper)
{
	float r = object->GetRadius() / CELL_SIZE;
	vec2d p = object->GetPos() / CELL_SIZE;

	_ASSERT(r > 0);

	//--------------------------------
	int xmin = (int) __max(0, p.x - r + 0.5f);
	int xmax = (int) __min((float) (_cx - 1), p.x + r + 0.5f);
	int ymin = (int) __max(0, p.y - r + 0.5f);
	int ymax = (int) __min((float) (_cy - 1), p.y + r + 0.5f);
	//-------------------------------------------------------------
	for( int x = xmin; x <= xmax; x++ )
	for( int y = ymin; y <= ymax; y++ )
	{
		if( oper )
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

// в конструкторе нельзя создавать игровые объекты
Level::Level()
{
	TRACE("Constructing the level\n");
	srand( GetTickCount() );

	_time        = 0;
	_timeBuffer  = 0;
	_limitHit    = false;
	_paused      = false;

	_safeMode    = true;

	_locations_x  = 0;
	_locations_y  = 0;
	_sx = _sy    = 0;

	_seed   = 1;

	_server = NULL;
	_client = NULL;

	/////////////////////////
	#ifdef _DEBUG
	_bInitialized = FALSE;
	#endif
	#ifdef NETWORK_DEBUG
	_dwChecksum = 0;
	#endif


	// register config handlers
	g_conf.s_volume->eventChange.bind(&Level::OnChangeSoundVolume, this);
}

void Level::Init(int X, int Y)
{
	Resize(X, Y);

	//
	// other objects
	//

	_Background::CreateInstance();
	_MessageArea::CreateInstance();

	new GC_Camera((GC_Player *) NULL);

	_temporaryText = new GC_Text(0, 0, "");
	_temporaryText->Show(false);
}

void Level::Resize(int X, int Y)
{
	_locations_x  = X * CELL_SIZE / LOCATION_SIZE + ((X * CELL_SIZE) % LOCATION_SIZE != 0 ? 1 : 0);
	_locations_y  = Y * CELL_SIZE / LOCATION_SIZE + ((Y * CELL_SIZE) % LOCATION_SIZE != 0 ? 1 : 0);
	_sx           = (float) X * CELL_SIZE;
	_sy	          = (float) Y * CELL_SIZE;

	for( int i = 0; i < Z_COUNT; i++ )
		z_grids[i].resize(_locations_x, _locations_y);

	grid_rigid_s.resize(_locations_x, _locations_y);
	grid_walls.resize(_locations_x, _locations_y);
	grid_wood.resize(_locations_x, _locations_y);
	grid_water.resize(_locations_x, _locations_y);
	grid_pickup.resize(_locations_x, _locations_y);

	_field.Resize(X + 1, Y + 1);
}

BOOL Level::init_emptymap()
{
	_ASSERT(!_bInitialized);
	_ASSERT(_bInitialized = TRUE);

	_gameType   = GT_EDITOR;
	_modeEditor = true;

	g_render->SetAmbient( 1.0f );

	_Background::Inst()->EnableGrid( g_conf.ed_drawgrid->Get() );
	_ThemeManager::Inst().ApplyTheme(0);

	return TRUE;
}

BOOL Level::init_import_and_edit(const char *mapName)
{
	_ASSERT(!_bInitialized);
	_ASSERT(_bInitialized = TRUE);

	_gameType   = GT_EDITOR;
	_modeEditor = true;

	g_render->SetAmbient( 1.0f );

	if( !Import(mapName) )
		return FALSE;

	Pause(true);

	return true;
}

BOOL Level::init_newdm(const char *mapName)
{
	_ASSERT(!_bInitialized);
	_ASSERT(_bInitialized = TRUE);

	_gameType   = GT_DEATHMATCH;
	_modeEditor = false;
	_seed       = rand();

	// score table
	new GC_TextScore();

	g_render->SetAmbient( g_conf.sv_nightmode->Get() ? 0.0f : 1.0f );

	return Import(mapName);
}

BOOL Level::init_load(const char *fileName)
{
	_ASSERT(!_bInitialized);
	_ASSERT(_bInitialized = TRUE);		// _ASSERT здесь чтобы не писать #ifdef _DEBUG

	_gameType   = GT_DEATHMATCH;
	_modeEditor = false;

	new GC_TextScore();

	return Unserialize(fileName);
}

Level::~Level()
{
	_ASSERT(IsSafeMode());

	TRACE("Destroying the level\n");

	SAFE_KILL(_temporaryText);

	for( OBJECT_LIST::iterator i = objects.begin(); i != objects.end(); )
	{
		GC_Object *pDelObj = *i;
		pDelObj->AddRef();
		pDelObj->Kill();
		i++; // must be called before Release()
		pDelObj->Release();
	}

	if( _client )
	{
		_client->ShutDown();
		delete _client;
		_client = NULL;
	}

	if( _server )
	{
		_server->ShutDown();
		delete _server;
		_server = NULL;
	}

	// unregister config handlers
	g_conf.s_volume->eventChange.clear();


	//-------------------------------------------
	_ASSERT(!g_env.nNeedCursor);
	_ASSERT(objects.empty());
}

// уровень должен быть пустым
bool Level::Unserialize(const char *fileName)
{
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

		_gameType = sh.dwGameType;

		g_conf.sv_timelimit->SetFloat(sh.timelimit);
		g_conf.sv_fraglimit->SetInt(sh.fraglimit);
		g_conf.sv_nightmode->Set(sh.nightmode);

		_time = sh.time;
		Init(sh.width, sh.height);

		while( sh.nObjects > 0 )
		{
			GC_Object::CreateFromFile(f);
			sh.nObjects--;
		}


		//восстанавливаем связи
		if( !f.RestoreAllLinks() )
			throw "ERROR: invalid links";

		// применение темы
		_infoTheme = sh.theme;
		_ThemeManager::Inst().ApplyTheme(_ThemeManager::Inst().FindTheme(sh.theme));


		FOREACH( players, GC_Player, pPlayer )
		{
			pPlayer->UpdateSkin();
		}

		GC_Camera::SwitchEditor();
		Pause(false);
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
		sh.fraglimit    = g_conf.sv_fraglimit->GetInt();
		sh.timelimit    = g_conf.sv_timelimit->GetFloat();
		sh.nightmode    = g_conf.sv_nightmode->Get();
		sh.time         = _time;
		sh.width        = (int) _sx / CELL_SIZE;
		sh.height       = (int) _sy / CELL_SIZE;
		sh.nObjects		= 0;	// будем увеличивать по мере записи

		WriteFile(f._file, &sh, sizeof(SaveHeader), &bytesWritten, NULL);
		if( bytesWritten != sizeof(SaveHeader) )
			throw "ERROR: couldn't write file. check disk space";

		//перебираем все объекты. если нужно - сохраняем
		OBJECT_LIST::reverse_iterator it = objects.rbegin();
		for( ; it != objects.rend(); ++it )
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

bool Level::Import(const char *fileName)
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

    Init(width, height);

	while( file.NextObject() )
	{
		float x, y;
		if( !file.getObjectAttribute("x", x) ) continue;
		if( !file.getObjectAttribute("y", y) ) continue;
		name2type::iterator it = get_n2t().find(file.getCurrentClassName());
		if( get_n2t().end() == it ) continue;
		GC_Object *object = get_t2i()[it->second].Create(x, y);
		object->mapExchange(file);
	}

	GC_Camera::SwitchEditor();
	return true;
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


	//
	// objects
	//

	if( !file.Open(fileName, true) )
		return false;

	FOREACH( objects, GC_Object, object )
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

void Level::Pause(bool pause)
{
//	if( _limitHit || _modeEditor ) return;

	_paused = pause;

	FOREACH( sounds, GC_Sound, pSound )
	{
		pSound->Freeze(pause);
	}
}

void Level::ToggleEditorMode()
{
	if( _modeEditor )
	{
		_modeEditor = false;
		_Background::Inst()->EnableGrid(false);
		Pause(false);
	}
	else
	{
		_modeEditor = true;
		_Background::Inst()->EnableGrid(g_conf.ed_drawgrid->Get());
		Pause(true);
	}
	GC_Camera::SwitchEditor();
}

GC_Object* Level::CreateObject(ObjectType type, float x, float y)
{
	_ASSERT(IsRegistered(type));
	return get_t2i()[type].Create(x, y);
}

GC_2dSprite* Level::PickEdObject(const vec2d &pt)
{
	for( int i = Z_COUNT - 1; i--; )
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
							&& ( !g_conf.ed_uselayers->Get() || GetTypeInfoByIndex(
							g_conf.ed_object->GetInt()).layer == GetTypeInfoByIndex(i).layer ) )
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
	_seed = (69069 * g_level->_seed + 1);
	return _seed & RAND_MAX;
}

float Level::net_frand(float max)
{
	return (float) net_rand() / RAND_MAX * max;
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
	float vt = tv.Length();

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
	                                GC_RigidBodyStatic* pIgnore,
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

	float la = a.Length();
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

		while(1)
		{
			if( curx >= 0 && curx < _locations_x &&
				cury >= 0 && cury < _locations_y )
			{
				OBJECT_LIST &tmp_list = list(i, curx, cury);
				for( OBJECT_LIST::iterator it = tmp_list.begin(); it != tmp_list.end(); ++it )
				{
					GC_RigidBodyStatic *object = (GC_RigidBodyStatic *) *it;
					if( object->Trace0() ) continue;
					if( object == pIgnore || object->IsKilled() ) continue;

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
		}
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
	if( _limitHit ) return;

	dt *= g_conf.sv_speed->GetFloat() / 100.0f;
	_ASSERT(dt >= 0);

	if( _paused )
		return;

	_ASSERT(!_modeEditor);

	_safeMode = false;

	if( _client )
	{
		//
		// network mode
		//

		float fixed_dt = 1.0f / g_conf.sv_fps->GetFloat() / NET_MULTIPLER;
		_timeBuffer += dt;
		if( _timeBuffer > 0 )
		do
		{
			//
			// обработка команд кадра
			//
			DataBlock db;
			while( _client->GetData(db) )
			{
				switch( db.type() )
				{
					case DBTYPE_TEXTMESSAGE:
						_MessageArea::Inst()->message( (LPCTSTR) db.data() );
						break;
					case DBTYPE_SERVERQUIT:
						_MessageArea::Inst()->message( "Сервер вышел" );
						break;
					case DBTYPE_PLAYERQUIT:
					{
						const DWORD &id = db.cast<DWORD>();
						OBJECT_LIST::iterator it = players.begin();
						while( it != players.end() )
						{
							if( GC_PlayerRemote *p = dynamic_cast<GC_PlayerRemote*>(*it) )
							{
								if( p->GetNetworkId() == id )
								{
									_MessageArea::Inst()->message( "игрок вышел" );
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
							_client->_ctrlBuf.push( ((ControlPacket *) db.data())[i] );
					} break;
				} // end of switch( db.type )

				if( DBTYPE_CONTROLPACKET == db.type() ) break;
			}

			if( _client->_ctrlBuf.empty() )
			{
				_timeBuffer = 0;
				break;	// нет кадра. пропускаем
			}


			//
			// ок, кадр получен. расчет игровой ситуации
			//

			#ifdef NETWORK_DEBUG
			DWORD dwCheckSum = 0, tmp_cs;
			#endif


			_time       += fixed_dt;
			_timeBuffer -= fixed_dt;
			OBJECT_LIST::safe_iterator it = ts_fixed.safe_begin();
			while( it != ts_fixed.end() )
			{
				GC_Object* pTS_Obj = *it;
				_ASSERT(!pTS_Obj->IsKilled());

				// расчет контрольной суммы для обнаружения потери синхронизации
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

#ifdef NETWORK_DEBUG
			_dwChecksum = dwCheckSum;
#endif
		} while(0);
	}
	else // if( _client )
	{
		int count = int(dt / MAX_DT_FIXED) + 1;
		float fixed_dt = dt / (float) count;

		do
		{
			_time += fixed_dt;
			OBJECT_LIST::safe_iterator it = ts_fixed.safe_begin();
			while( it != ts_fixed.end() )
			{
				GC_Object* pTS_Obj = *it;
				_ASSERT(!pTS_Obj->IsKilled());
				pTS_Obj->TimeStepFixed(fixed_dt);
				++it;
			}
			GC_RigidBodyDynamic::ProcessResponse(fixed_dt);
		} while( --count );
	}

	OBJECT_LIST::safe_iterator it = ts_floating.safe_begin();
	while( it != ts_floating.end() )
	{
		GC_Object* pTS_Obj = *it;
		_ASSERT(!pTS_Obj->IsKilled());
		pTS_Obj->TimeStepFloat(dt);
		++it;
	}

	_safeMode = true;
}

void Level::Render() const
{
	//
	// определение активных камер и выбор конфигурации дисплея
	//

	GC_Camera *pMaxShake = NULL;

	if( g_render->GetWidth() >= (int) g_level->_sx &&
		g_render->GetHeight() >= (int) g_level->_sy )
	{
		float max_shake = 0;
		FOREACH( cameras, GC_Camera, pCamera )
		{
			if( !pCamera->IsActive() ) continue;
			if( pCamera->GetShake() >= max_shake )
			{
				pMaxShake = pCamera;
				max_shake = pCamera->GetShake();
			}
		}
	}

	int count = 0;
	FOREACH( cameras, GC_Camera, pCamera )
	{
		if( pMaxShake ) pCamera = pMaxShake;
		if( !pCamera->IsActive() ) continue;


		//
		// рендеринг освещения
		//

		g_render->SetMode(RM_LIGHT);
		pCamera->Select();
		if( g_conf.sv_nightmode->Get() )
		{
			float xmin = (float) __max(0, g_env.camera_x );
			float ymin = (float) __max(0, g_env.camera_y );
			float xmax = __min(g_level->_sx, (float) g_env.camera_x +
				(float) g_render->GetViewportWidth() / pCamera->_zoom );
			float ymax = __min(g_level->_sy, (float) g_env.camera_y +
				(float) g_render->GetViewportHeight() / pCamera->_zoom );

			FOREACH( lights, GC_Light, pLight )
			{
				_ASSERT(!pLight->IsKilled());
				if( pLight->GetPos().x + pLight->GetRenderRadius() > xmin &&
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
		_Background::Inst()->Draw();

		for( int z = 0; z < Z_COUNT; ++z )
		{
			// loop over grid sets
			for( int lev = 0; lev < 4; ++lev )
			{
				static const int dx[] = {0, LOCATION_SIZE/2, 0, LOCATION_SIZE/2};
				static const int dy[] = {0, 0, LOCATION_SIZE/2, LOCATION_SIZE/2};

				int xmin = __max(0, (g_env.camera_x - dx[lev]) / LOCATION_SIZE);
				int ymin = __max(0, (g_env.camera_y - dy[lev]) / LOCATION_SIZE);
				int xmax = __min(g_level->_locations_x - 1,
					((g_env.camera_x + int((float) g_render->GetViewportWidth() /
					pCamera->_zoom)) - dx[lev]) / LOCATION_SIZE);
				int ymax = __min(g_level->_locations_y - 1,
					((g_env.camera_y + int((float) g_render->GetViewportHeight() /
					pCamera->_zoom)) - dy[lev]) / LOCATION_SIZE);

				for( int x = xmin; x <= xmax; ++x )
				for( int y = ymin; y <= ymax; ++y )
				{
					FOREACH( z_grids[z](lev).element(x,y), GC_2dSprite, object )
					{
						_ASSERT(!object->IsKilled());
						object->Draw();
						_ASSERT(!object->IsKilled());
					}
				}
			} // loop over grid sets


			// loop over globals
			FOREACH( z_globals[z], GC_2dSprite, object )
			{
				_ASSERT(!object->IsKilled());
				object->Draw();
				_ASSERT(!object->IsKilled());
			}
		}

		if( pMaxShake ) break;
	}	// cameras

/*
	//
	// paint Z_SCREEN layer
	//

	if( !thumbnail )
	{
		g_render->SetMode(RM_INTERFACE);

		FOREACH( z_globals[Z_COUNT-1], GC_2dSprite, object )
		{
			_ASSERT(!object->IsKilled());
			object->Draw();
			_ASSERT(!object->IsKilled());
		}
	}
*/
}

GC_Object* Level::FindObject(const char *name) const
{
	std::map<string_t, const GC_Object*>::const_iterator it = _nameToObjectMap.find(name);
	return _nameToObjectMap.end() != it ? const_cast<GC_Object*>(it->second) : NULL;
}

void Level::OnChangeSoundVolume()
{
	FOREACH( sounds, GC_Sound, pSound )
	{
		pSound->UpdateVolume();
	}
}


///////////////////////////////////////////////////////////////////////////////
// end of file
