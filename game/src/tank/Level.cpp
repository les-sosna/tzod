// Level.cpp
////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Level.h"
//#include "MainLoop.h"
#include "options.h"
#include "macros.h"
#include "functions.h"

#include "core/debug.h"

#include "network/TankClient.h"
#include "network/TankServer.h"

#include "video/RenderBase.h"
#include "video/TextureManager.h" // for ThemeManager

#include "fs/SaveFile.h"
#include "fs/MapFile.h"

#include "gc/GameClasses.h"
#include "gc/RigidBody.h"
#include "gc/indicators.h"
#include "gc/editor.h"
#include "gc/Player.h"
#include "gc/Sound.h"
#include "gc/pickup.h"


////////////////////////////////////////////////////////////
unsigned long FieldCell::_sessionId;

void FieldCell::UpdateProperties()
{
	_prop = 0;
	for( int i = 0; i < _objCount; i++ )
	{
		_ASSERT(_ppObjects[i]->GetProperties() > 0);
		if( _ppObjects[i]->GetProperties() > _prop )
			_prop = _ppObjects[i]->GetProperties();
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
	FRECT rect;
	object->GetAABB(&rect);
	//--------------------------------
	int xmin = (int) __max(0, rect.left / CELL_SIZE + 0.5f);
	int xmax = (int) __min((float) (_cx - 1), rect.right / CELL_SIZE + 0.5f);
	int ymin = (int) __max(0, rect.top / CELL_SIZE + 0.5f);
	int ymax = (int) __min((float) (_cy - 1), rect.bottom / CELL_SIZE + 0.5f);
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
	REPORT("==== Field dump ====\n");

	for( int y = 0; y < _cy; y++ )
	{
		for( int x = 0; x < _cx; x++ )
		{
			switch( (*this)(x, y).Properties() )
			{
			case 0:
				REPORT(" ");
				break;
			case 1:
				REPORT("-");
				break;
			case 0xFF:
				REPORT("#");
				break;
			}
		}
		REPORT("\n");
	}

	REPORT("=== end of dump ====\n");
}

#endif

////////////////////////////////////////////////////////////

// в конструкторе уровня нельзя создавать игровые объекты
Level::Level()
{
	LOGOUT_1("enter to the level constructor\n");
	srand( GetTickCount() );

	_timer.SetMaxDt(MAX_DT / (float) OPT(gameSpeed) * 100.0f);
	_time        = 0;
	_timeBuffer  = 0;
	_limitHit    = false;

	_locations_x  = 0;
	_locations_y  = 0;
	_sx = _sy    = 0;

	_seed = 1;

	_server = NULL;
	_client = NULL;

	/////////////////////////
	#ifdef _DEBUG
	_bInitialized = FALSE;
	#endif
	#ifdef NETWORK_DEBUG
	_dwChecksum = 0;
	#endif
}

void Level::Init(int X, int Y)
{
	Resize(X, Y);

	//
	// other objects
	//

	_Background::CreateInstance();
//	_Cursor::CreateInstance();
	_FpsCounter::CreateInstance();
	_MessageArea::CreateInstance();


	new GC_Winamp();
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

	for( int i = 0; i < Z_COUNT-1; i++ )
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


	OPT(gameType)    = GT_EDITOR;
	OPT(bModeEditor) = true;

	OPT(bNightMode)  = false;

	_Editor::CreateInstance();
	g_render->SetAmbient( 1.0f );

	_Background::Inst()->EnableGrid(OPT(bShowGrid));
	_ThemeManager::Inst().ApplyTheme(0);

	return TRUE;
}

BOOL Level::init_import_and_edit(char *mapName)
{
	_ASSERT(!_bInitialized);
	_ASSERT(_bInitialized = TRUE);

	OPT(gameType)    = GT_EDITOR;
	OPT(bModeEditor) = true;

	OPT(bNightMode)  = false;

	_Editor::CreateInstance();
	g_render->SetAmbient( 1.0f );

	return Import(mapName, true);
}

BOOL Level::init_newdm(const char *mapName)
{
	_ASSERT(!_bInitialized);
	_ASSERT(_bInitialized = TRUE);

	g_options.gameType    = GT_DEATHMATCH;
	g_options.bModeEditor = false;

	// time indicator
	new GC_TextTime(g_render->getXsize() - 1, g_render->getYsize() - 1, alignTextRB);

	// score table
	new GC_TextScore();

	//editor object
	_Editor::CreateInstance();
	g_render->SetAmbient(OPT(bNightMode) ? 0.0f : 1.0f );

	bool res = Import(mapName, false);

	_timer.Start();

	return res;
}

BOOL Level::init_load(const char *fileName)
{
	_ASSERT(!_bInitialized);
	_ASSERT(_bInitialized = TRUE);		// _ASSERT здесь чтобы не писать #ifdef _DEBUG

	g_options.gameType    = GT_DEATHMATCH;
	g_options.bModeEditor = false;

	new GC_TextScore();
	new GC_TextTime(g_render->getXsize() - 1, g_render->getYsize() - 1, alignTextRB);

	_Editor::CreateInstance();

	return Unserialize(fileName);
}

Level::~Level()
{
	LOGOUT_1("Level::~Level()\n");

	SAFE_KILL(_temporaryText);

	for( OBJECT_LIST::iterator i = objects.begin(); i != objects.end(); )
	{
		GC_Object *pDelObj = *i;
		pDelObj->AddRef();
		pDelObj->Kill();
		i++;	// должно вызываться СТРОГО перед Release()
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

	//-------------------------------------------
	_ASSERT(!g_env.nNeedCursor);
	_ASSERT(!objects.size());
}

// уровень должен быть пустым
bool Level::Unserialize(const char *fileName)
{
	LOGOUT_2("Loading level from file '%s'...\n", fileName);

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
		REPORT("Load error: file open failed\n");
		return false;
	}

	bool result = true;
	try
	{
		DWORD bytesRead = 0;

		SAVEHEADER sh = {0};
		ReadFile(f._file, &sh, sizeof(SAVEHEADER), &bytesRead, NULL);
		if( sizeof(SAVEHEADER) != bytesRead )
			throw "Load error: invalid file size\n";

		if( VERSION != sh.dwVersion )
			throw "Load error: invalid version\n";

		g_options.gameType   = sh.dwGameType;
		g_options.timelimit  = sh.timelimit;
		g_options.fraglimit  = sh.fraglimit;
		g_options.bNightMode = sh.nightmode;

		_time = sh.fTime;
		Init(sh.X, sh.Y);

		while( sh.nObjects > 0 )
		{
			GC_Object::CreateFromFile(f);
			sh.nObjects--;
		}


		//восстанавливаем связи
		LOGOUT_1("restoring links...\n");
		if( !f.RestoreAllLinks() )
			throw "Load error: invalid links\n";

		// применение темы
		_infoTheme = sh.theme;
		_ThemeManager::Inst().ApplyTheme(_ThemeManager::Inst().FindTheme(sh.theme));


		ENUM_BEGIN(players, GC_Player, pPlayer) {
			pPlayer->UpdateSkin();
		} ENUM_END();

		GC_Camera::SwitchEditor();
		Pause(false);

		REPORT("Load OK\n");
	}
	catch (const char *msg)
	{
		REPORT(msg);
		result = false;
	}

	CloseHandle(f._file);
	return result;
}

bool Level::Serialize(const char *fileName)
{
	REPORT("Saving level...\n");

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
		REPORT("Save error: file open failed\n");
		return false;
	}

	bool result = true;
    try
	{
		// AI_Controller захватывает множество объектов. При этом, сам
		// он не сохраняется. Чтобы устранить утечку памяти после загрузки
		// уровня, необходимо принудительно освободить объекты

		ENUM_BEGIN(players, GC_Player, pPlayer)
		{
			if (pPlayer->IsKilled()) continue;
			if (pPlayer->_controller)
				pPlayer->_controller->Reset();
		} ENUM_END();


		// сброс состояния редактора для освобождения занятых объектов
		if( _Editor::Inst() )
			_Editor::Inst()->Reset();


		SAVEHEADER sh = {0};
		strcpy(sh.theme, _infoTheme.c_str());
		sh.dwVersion    = VERSION;
		sh.dwGameType   = g_options.gameType;
		sh.fraglimit    = g_options.fraglimit;
		sh.timelimit    = g_options.timelimit;
		sh.nightmode    = g_options.bNightMode;
		sh.fTime        = _time;
		sh.X            = (int) _sx / CELL_SIZE;
		sh.Y            = (int) _sy / CELL_SIZE;
		sh.nObjects		= 0;	// будем увеличивать по мере записи

		WriteFile(f._file, &sh, sizeof(SAVEHEADER), &bytesWritten, NULL);
		if( bytesWritten != sizeof(SAVEHEADER) )
			throw "Save error\n";

		//перебираем все объекты. если нужно - сохраняем
		OBJECT_LIST::reverse_iterator it = g_level->objects.rbegin();
		for( ; it != g_level->objects.rend(); ++it )
		{
			GC_Object *object = *it;
			if( object->IsSaved() )
			{
				ObjectType type = object->GetType();
				WriteFile(f._file, &type, sizeof(type), &bytesWritten, NULL);
				LOGOUT_3("saving object %d at 0x%X\n", type, 
					SetFilePointer(f._file, 0, 0, FILE_CURRENT));
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
					throw "Save error: serialize object\n";
				}
				sh.nObjects++;
			}
		}

		// return to begin of file and write count of saved objects
		SetFilePointer(f._file, 0, NULL, FILE_BEGIN);
		WriteFile(f._file, &sh, sizeof(SAVEHEADER), &bytesWritten, NULL);
		if( bytesWritten != sizeof(SAVEHEADER) )
			throw "Save error\n";
	}
	catch(const char *msg)
	{
		REPORT(msg);
		result = false;
	}
	CloseHandle(f._file);

	REPORT("Save OK\n");
	return result;
}

bool Level::Import(const char *fileName, bool bForEditor)
{
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
		GC_Editor::CreateObject(file);

	GC_Camera::SwitchEditor();
	return true;
}

bool Level::Export(const char *fileName)
{
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

	ENUM_BEGIN(objects, GC_Object, object)
	{
		if( object->IsKilled() ) continue;
		if( GC_Editor::isRegistered(object->GetType()) )
		{
			file.BeginObject(GC_Editor::GetName(object->GetType()));
			object->mapExchange(file);
			if( !file.WriteCurrentObject() ) return false;
		}
	} ENUM_END();


	//
	// thumbnail
	//

//	bool ed = OPT(bModeEditor);
//	OPT(bModeEditor) = false;
//	RenderFrame(true);
//	OnPrintScreen();
//	OPT(bModeEditor) = ed;

	return true;
}

void Level::Pause(bool pause)
{
	if( _limitHit || OPT(bModeEditor) ) return;

	ENUM_BEGIN(sounds, GC_Sound, pSound) {
		pSound->Freeze(pause);
	} ENUM_END();
}

GC_2dSprite* Level::PickEdObject(const vec2d &pt)
{
	for( int i = Z_COUNT - 1; i--; )
	{
		std::vector<OBJECT_LIST*> receive;
		g_level->z_grids[i].OverlapCircle(receive,
			pt.x / LOCATION_SIZE, pt.y / LOCATION_SIZE, 0);

		std::vector<OBJECT_LIST*>::iterator rit = receive.begin();
		for( ; rit != receive.end(); rit++ )
		{
			OBJECT_LIST::iterator it = (*rit)->begin();
			for( ; it != (*rit)->end(); ++it )
			{
				GC_2dSprite *object = (GC_2dSprite *) (*it);

				FRECT frect;
				object->GetGlobalRect(frect);

				if( PtInFRect(frect, pt) )
				{
					for( int i = 0; i < _Editor::Inst()->GetObjectCount(); ++i )
					{
						if( !OPT(bUseLayers) || _Editor::Inst()->GetLayer(
							g_options.nCurrentObject) == _Editor::Inst()->GetLayer(i) )
						if( object->GetType() == _Editor::Inst()->GetOwnedType(i) )
						{
							if( dynamic_cast<GC_Weapon *>(object) )
								if( ((GC_Weapon *)object)->_bAttached )
									continue;
							return object;
						}
					}
				}
			}
		}
	}

	return NULL;
}

GC_RigidBodyStatic* Level::agTrace( GridSet<OBJECT_LIST> &list,
	                                 GC_RigidBodyStatic* pIgnore,
	                                 const vec2d &x0,      // координаты начала
	                                 const vec2d &a,       // направление
	                                 vec2d *ht,
	                                 vec2d *norm) const
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

	float la = a_tmp.Length();

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
			if( curx >= 0 && curx < g_level->_locations_x &&
				cury >= 0 && cury < g_level->_locations_y )
			{
				OBJECT_LIST &tmp_list = list(i, curx, cury);
				for( OBJECT_LIST::iterator it = tmp_list.begin(); it != tmp_list.end(); ++it )
				{
					GC_RigidBodyStatic *object = (GC_RigidBodyStatic *) *it;
					if( object->trace0() ) continue;
					if( object == pIgnore || object->IsKilled() ) continue;

					FRECT ColRect;
					object->GetAABB(&ColRect);

					vec2d m[4];
					m[0].Set( ColRect.left,  ColRect.top );
					m[1].Set( ColRect.right, ColRect.top );
					m[2].Set( ColRect.right, ColRect.bottom );
					m[3].Set( ColRect.left,  ColRect.bottom );


					bool bHasHit = false;

					//грубо
					int sgn = 0, oldsgn = 0;
					for (int n = 0; n < 4; n++)
					{
						float d = a.x * (m[n].y - x0.y) - a.y * (m[n].x - x0.x);

						if (d < 0)
							sgn = -1;
						else if (d > 0)
							sgn = 1;
						else
						{
							bHasHit = true;
							break;
						}

						if( 0 == n ) oldsgn = sgn;

						if( sgn != oldsgn )
						{
							bHasHit = true;
							break;
						}

						oldsgn = sgn;
					}

					// точно
					if( bHasHit )
					{
						for( int i = 0; i < 4; ++i )
						{
							m[i] = object->GetVertex(i);
						}

						for( int n = 0; n < 4; ++n )
						{
							float xb = m[n].x;
							float yb = m[n].y;

							float bx = m[(n+1)%4].x - xb;
							float by = m[(n+1)%4].y - yb;

							float delta = a.y*bx - a.x*by;
							if( delta <= 0 ) continue;

							float tb = (a.x*(yb - x0.y) - a.y*(xb - x0.x)) / delta;

							if( tb <= 1 && tb >= 0 )
							{
								float len = (bx*(yb - x0.y) - by*(xb - x0.x)) / delta;

								if (len >= 0 && len < minLen)
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

			if (curx == targx && cury == targy) break;

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
					a_tmp.y*((float) newx + 0.5f - cx)) / la;

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
	if( ( ((x0.x <= hit.x) && (hit.x <= x0.x + a.x))||((x0.x + a.x <= hit.x) && (hit.x <= x0.x)) ) &&
		( ((x0.y <= hit.y) && (hit.y <= x0.y + a.y))||((x0.y + a.y <= hit.y) && (hit.y <= x0.y)) ) )
	{
		if( norm )
		{
			float l = sqrtf(nx*nx + ny*ny);
			norm->Set(nx / l, ny / l);
		}
		if( ht ) *ht = hit;
		return pBestObject;
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

///////////////////////////////////////////////////////////////////////////////
// end of file
