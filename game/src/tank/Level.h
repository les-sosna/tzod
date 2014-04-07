// Level.h

#pragma once

#include "LevelInterfaces.h"
#include "ObjectListener.h"
//#include "gc/Object.h" // FIXME!

#include "network/ControlPacket.h"

#include "video/RenderBase.h"

#include "DefaultCamera.h"

#include "constants.h"

#include <core/PtrList.h>
#include <core/Grid.h>

#include <algorithm>
#include <map>
#include <vector>


#pragma region path finding stuff

namespace FS
{
	class Stream;
}
class ClientBase;
class GC_RigidBodyStatic;

class Field;
class FieldCell
{
public:
	static unsigned long _sessionId;    // идентификатор текущей сессии
	//-----------------------------
	short _x, _y;                       // координаты клетки
	//-----------------------------
	unsigned long _mySession;        // сессия, в которой данная клетка была проверена
	//-----------------------------
	GC_RigidBodyStatic **_ppObjects;
	unsigned char _objCount;
	unsigned char _prop;             // 0 - free, 1 - could be broken, 0xFF - impassable
	//-----------------------------
	void UpdateProperties();
	FieldCell()
	{
		_ppObjects   = NULL;
		_objCount    = 0;
		_prop        = 0;     // free
		_mySession   = 0xffffffff;
	}
public:

	const FieldCell *_prevCell;

	inline int GetObjectsCount() const { return _objCount; }
	inline GC_RigidBodyStatic* GetObject(int index) const { return _ppObjects[index]; }

	inline bool IsChecked() const { return _mySession == _sessionId; }
	inline void Check()           { _mySession = _sessionId;         }

	inline void UpdatePath(float before, short x, short y)
	{
		int dx = abs(x - GetX());
		int dy = abs(y - GetY());
		float pathAfter = (float) std::max(dx, dy) + (float) std::min(dx, dy) * 0.4142f;
		_before = before;
		_total = before + pathAfter;
	}

	void AddObject(GC_RigidBodyStatic *object);
	void RemoveObject(GC_RigidBodyStatic *object);

	inline short GetX()               const { return _x;    }
	inline short GetY()               const { return _y;    }
	inline unsigned char Properties() const { return _prop; }

	inline float Total() const { return _total; }
	inline float Before() const { return _before; }

private:
	float _before;          // path cost to this node
	float _total;           // total path cost estimate

	FieldCell(const FieldCell &other); // no copy
	FieldCell& operator = (const FieldCell &other);
};

class RefFieldCell
{
	FieldCell *_cell;
public:
	RefFieldCell(FieldCell &cell) { _cell = &cell; }
	operator FieldCell& () const { return *_cell; }
	bool operator > (const RefFieldCell &cell) const
	{
		return _cell->Total() > cell._cell->Total();
	}
};


class Field
{
	FieldCell _edgeCell;
	FieldCell **_cells;
	int _cx;
	int _cy;

	void Clear();

public:
	static void NewSession() { ++FieldCell::_sessionId; }

	Field();
	~Field();

	void Resize(int cx, int cy);
	void ProcessObject(GC_RigidBodyStatic *object, bool add);
	int GetX() const { return _cx; }
	int GetY() const { return _cy; }


#ifdef _DEBUG
	FieldCell& operator() (int x, int y);
	void Dump();
#else
	inline FieldCell& operator() (int x, int y)
	{
		return (x >= 0 && x < _cx && y >= 0 && y < _cy) ? _cells[y][x] : _edgeCell;
	}
#endif

};

#pragma endregion

///////////////////////////////////////////////////////////////////////////////

class GC_Object;
class GC_2dSprite;

struct PlayerDescEx;
struct BotDesc;


//////////////////////

struct IEditorModeListener
{
	virtual void OnEditorModeChanged(bool editorMode) = 0;
};

class EditorModeListenerHelper: private IEditorModeListener
{
protected:
	EditorModeListenerHelper();
	~EditorModeListenerHelper();
};

enum GlobalListID
{
	LIST_objects,
	LIST_services,
	LIST_respawns,
	LIST_projectiles,
	LIST_players,
	LIST_sounds,
	LIST_indicators,
	LIST_vehicles,
	LIST_pickups,
	LIST_lights,
	LIST_cameras,
	//------------------
	GLOBAL_LIST_COUNT
};

#define MAX_THEME_NAME  128

class Level
	: public ILevelController
{
	friend class GC_Object;

	std::map<const GC_Object*, std::string>  _objectToStringMaps[32];
	std::map<std::string, const GC_Object*>  _nameToObjectMap; // TODO: try to avoid name string duplication


	struct SaveHeader
	{
		uint32_t dwVersion;
		bool  nightmode;
		float timelimit;
		int   fraglimit;
		float time;
		int   width;
		int   height;
		char  theme[MAX_THEME_NAME];
	};

	PtrList<GC_Object> _objectLists[GLOBAL_LIST_COUNT];
	std::set<IEditorModeListener*> _editorModeListeners;
	bool    _modeEditor;

public:

#ifndef NDEBUG
	std::set<GC_Object*> _garbage;
#endif

#ifdef NETWORK_DEBUG
	uint32_t _checksum;
	int _frame;
	FILE *_dump;
#endif

	PtrList<GC_Object>& GetList(GlobalListID id) { return _objectLists[id]; }
	const PtrList<GC_Object>& GetList(GlobalListID id) const { return _objectLists[id]; }

	Grid<PtrList<GC_Object>>  grid_rigid_s;
	Grid<PtrList<GC_Object>>  grid_walls;
	Grid<PtrList<GC_Object>>  grid_wood;
	Grid<PtrList<GC_Object>>  grid_water;
	Grid<PtrList<GC_Object>>  grid_pickup;

	PtrList<GC_Object>     ts_fixed;

	// graphics
	PtrList<GC_Object>        z_globals[Z_COUNT];
	Grid<PtrList<GC_Object>>  z_grids[Z_COUNT];

	ObjectListener *_serviceListener;
	DefaultCamera _defaultCamera;

	size_t _texBack;
	size_t _texGrid;
	void DrawBackground(size_t tex) const;

/////////////////////////////////////
//settings
	bool    _frozen;
	bool    _limitHit;  // fraglimit or timelimit hit
	float   _sx, _sy;   // world size

	int _locationsX;
	int _locationsY;

	std::string _infoAuthor;
	std::string _infoEmail;
	std::string _infoUrl;
	std::string _infoDesc;
	std::string _infoTheme;
	std::string _infoOnInit;


/////////////////////////////////////////////////////
//network

	unsigned long _seed;

/////////////////////////////////////
public:
	float _time;

	void Step(const std::vector<ControlPacket> &ctrl, float dt);

	Field _field;

	bool  _safeMode;

/////////////////////////////////////////////////////
	Level();
	~Level();


	void AddEditorModeListener(IEditorModeListener *ls);
	void RemoveEditorModeListener(IEditorModeListener *ls);

	void Resize(int X, int Y);
	void HitLimit();


	bool init_emptymap(int X, int Y);
	bool init_import_and_edit(const char *mapName);


public:
	bool IsEmpty() const;

	void Unserialize(const char *fileName);
	void Serialize(const char *fileName);

	void Export(const SafePtr<FS::Stream> &file);
	void Import(const SafePtr<FS::Stream> &file);

	void PauseSound(bool pause);
	void Freeze(bool freeze) { _frozen = freeze; }

	void RunCmdQueue(float dt);
	void Render() const;
	bool IsSafeMode() const { return _safeMode; }
	bool IsGamePaused() const;
	GC_Object* FindObject(const std::string &name) const;

	int   net_rand();
	float net_frand(float max);
	vec2d net_vrand(float len);

	bool CalcOutstrip( const vec2d &fp,    // fire point
                       float vp,           // speed of the projectile
                       const vec2d &tx,    // target position
                       const vec2d &tv,    // target velocity
                       vec2d &out_fake );  // out: fake target position


	void RenderInternal(const FRECT &world) const;


	//
	// tracing
	//

public:
	struct CollisionPoint
	{
		GC_RigidBodyStatic *obj;
		vec2d normal;
		float enter;
		float exit;
	};

	GC_RigidBodyStatic* TraceNearest( Grid<PtrList<GC_Object>> &list,
	                             const GC_RigidBodyStatic* ignore,
	                             const vec2d &x0,      // origin
	                             const vec2d &a,       // direction and length
	                             vec2d *ht   = NULL,
	                             vec2d *norm = NULL) const;

	void TraceAll( Grid<PtrList<GC_Object>> &list,
	               const vec2d &x0,      // origin
	               const vec2d &a,       // direction and length
	               std::vector<CollisionPoint> &result) const;

	template<class SelectorType>
	void RayTrace(Grid<PtrList<GC_Object>> &list, SelectorType &s) const;


	//
	// editor
	//

	bool GetEditorMode() const { return _modeEditor; }
	void SetEditorMode(bool editorModeEnable);
	GC_2dSprite* PickEdObject(const vec2d &pt, int layer);


	//
	// config callback handlers
	//

protected:
	void OnChangeSoundVolume();
	void OnChangeNightMode();

	//
	// debugging routines
	//
public:
	mutable std::vector<MyLine> _dbgLineBuffer;
	void DbgLine(const vec2d &v1, const vec2d &v2, SpriteColor color = 0x00ff00ff) const
#ifdef NDEBUG
	{} // do nothing in release mode
#endif
		;


	// ILevelController
	virtual void Clear();
	virtual PlayerHandle* GetPlayerByIndex(size_t playerIndex);
	virtual void SetPlayerInfo(PlayerHandle *p, const PlayerDesc &pd);
	virtual void PlayerQuit(PlayerHandle *p);
	virtual PlayerHandle* AddHuman(const PlayerDesc &pd);
	virtual void AddBot(const BotDesc &bd);
	virtual void init_newdm(FS::Stream *s, unsigned long seed);

	virtual float GetTime() const { return _time; }

#ifdef NETWORK_DEBUG
	virtual uint32_t GetChecksum() const { return _checksum; }
	virtual unsigned int GetFrame() const { return _frame; }
#endif
};

///////////////////////////////////////////////////////////////////////////////

// inline functions definition
//#include "Level.inl"

///////////////////////////////////////////////////////////////////////////////
// end of file
