// Level.h

#pragma once

#include "ObjectListener.h"
#include "gc/Object.h" // FIXME!
#include "gc/RigidBody.h"

#include "network/ControlPacket.h"

#include "video/RenderBase.h"

#include "DefaultCamera.h"


#pragma region path finding stuff

namespace FS
{
	class Stream;
}

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
		float pathAfter = (float) __max(dx, dy) + (float) __min(dx, dy) * 0.4142f;
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

#define ED_SERVICE(name, desc) Level::RegisterService<__ThisClass>((name), (desc))

#define ED_ACTOR(name, desc, layer, width, height, align, offset)   \
	Level::RegisterActor<__ThisClass>(                              \
	(name), (desc), (layer), (width), (height), (align), (offset) )

#define ED_ITEM(name, desc, layer) ED_ACTOR(name, desc, layer, 2, 2, CELL_SIZE/2, 0)
#define ED_LAND(name, desc, layer) ED_ACTOR(name, desc, layer, CELL_SIZE, CELL_SIZE, CELL_SIZE, CELL_SIZE/2)
#define ED_TURRET(name, desc) ED_ACTOR(name, desc, 0, CELL_SIZE*2, CELL_SIZE*2, CELL_SIZE, CELL_SIZE)

///////////////////////////////////////////////////////////////////////////////

class GC_Object;
class GC_2dSprite;


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

class Level
{
	friend class GC_Object;

	std::map<const GC_Object*, string_t>  _objectToStringMaps[32];
	std::map<string_t, const GC_Object*>  _nameToObjectMap; // TODO: try to avoid name string duplication


	struct SaveHeader
	{
		DWORD dwVersion;
		DWORD dwGameType;
		bool  nightmode;
		float timelimit;
		int   fraglimit;
		float time;
		int   width;
		int   height;
		char  theme[MAX_PATH];
	};

	ObjectList _objectLists[GLOBAL_LIST_COUNT];
	std::set<IEditorModeListener*> _editorModeListeners;
	bool    _modeEditor;

public:

#ifndef NDEBUG
	std::set<GC_Object*> _garbage;
#endif

#ifdef NETWORK_DEBUG
	DWORD _checksum;
	int _frame;
	FILE *_dump;
#endif

	ObjectList& GetList(GlobalListID id) { return _objectLists[id]; }
	const ObjectList& GetList(GlobalListID id) const { return _objectLists[id]; }

	Grid<ObjectList>  grid_rigid_s;
	Grid<ObjectList>  grid_walls;
	Grid<ObjectList>  grid_wood;
	Grid<ObjectList>  grid_water;
	Grid<ObjectList>  grid_pickup;

	ObjectList     ts_fixed;

	// graphics
	ObjectList        z_globals[Z_COUNT];
	Grid<ObjectList>  z_grids[Z_COUNT];

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

	DWORD    _gameType;

	string_t _infoAuthor;
	string_t _infoEmail;
	string_t _infoUrl;
	string_t _infoDesc;
	string_t _infoTheme;
	string_t _infoOnInit;


/////////////////////////////////////////////////////
//network

	unsigned long _seed;

/////////////////////////////////////
public:
	std::deque<float> _dt;

	int _steps;
	float _time;
	float _timeBuffer;

	void Step(const ControlPacketVector &ctrl, float dt);

	Field _field;

	int  _ctrlSentCount;
	bool  _safeMode;

/////////////////////////////////////////////////////
	Level();
	~Level();


	void AddEditorModeListener(IEditorModeListener *ls);
	void RemoveEditorModeListener(IEditorModeListener *ls);

	void Resize(int X, int Y);
	void Clear();

	void HitLimit();


	bool init_emptymap(int X, int Y);
	bool init_import_and_edit(const char *mapName);

	void init_newdm(const SafePtr<FS::Stream> &s, unsigned long seed);


public:
	bool IsEmpty() const;

	bool RestoreObject(ObjectType otType, HANDLE file);

	void Unserialize(const char *fileName);
	void Serialize(const char *fileName);

	void Export(const SafePtr<FS::Stream> &file);
	void Import(const SafePtr<FS::Stream> &file, bool execInitScript);

	void PauseSound(bool pause);
	void Freeze(bool freeze) { _frozen = freeze; }

	void RunCmdQueue(float dt);
	void TimeStep(float dt);
	void Render() const;
	bool IsSafeMode() const { return _safeMode; }
	bool IsGamePaused() const;
	GC_Object* FindObject(const string_t &name) const;

	int   net_rand();
	float net_frand(float max);
	vec2d net_vrand(float len);

	bool CalcOutstrip( const vec2d &fp,    // fire point
                       float vp,           // speed of the projectile
                       const vec2d &tx,    // target position
                       const vec2d &tv,    // target velocity
                       vec2d &out_fake );  // out: fake target position


	void RenderInternal(const FRECT &world) const;


	class AbstractClient
	{
		std::deque<ControlPacketVector> _cpv;
	public:
		void Reset() {_cpv.clear();}
		bool Recv(ControlPacketVector &result);
		void Send(std::vector<VehicleState> &ctrl
#ifdef NETWORK_DEBUG
		          , DWORD cs, int frame
#endif
		         );
		float GetBoost() const;
	} _abstractClient;




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

	GC_RigidBodyStatic* TraceNearest( Grid<ObjectList> &list,
	                             const GC_RigidBodyStatic* ignore,
	                             const vec2d &x0,      // origin
	                             const vec2d &a,       // direction and length
	                             vec2d *ht   = NULL,
	                             vec2d *norm = NULL) const;

	void TraceAll( Grid<ObjectList> &list,
	               const vec2d &x0,      // origin
	               const vec2d &a,       // direction and length
	               std::vector<CollisionPoint> &result) const;

	template<class SelectorType>
	void RayTrace(Grid<ObjectList> &list, SelectorType &s) const;


	//
	// editing interface
	//
#pragma region Editor
private:
	typedef GC_Object* (*CreateProc) (float, float);
	
	template<class T>
	static GC_Object* CreateActor(float x, float y)
	{
		return new T(x, y);
	}

	template<class T>
	static GC_Object* CreateService(float x, float y)
	{
		return new T();
	}

	struct EdItem
	{
		CreateProc    Create;
		int           layer;
		float         align;
		float         offset;
		vec2d         size;
		bool          service;
		const char*   name;
		const char*   desc;
	};

	typedef std::map<ObjectType, EdItem> type2item;
	typedef std::map<string_t, ObjectType> name2type;
	typedef std::vector<ObjectType> index2type;

	static type2item& get_t2i()
	{
		static type2item t2i;
		return t2i;
	}
	static name2type& get_n2t()
	{
		static name2type n2t;
		return n2t;
	}
	static index2type& get_i2t()
	{
		static index2type i2t; // sort by desc
		return i2t;
	}

	static bool CompareTypes(ObjectType left, ObjectType right)
	{
		return 0 <= strcmp(get_t2i()[left].desc, get_t2i()[right].desc);
	}
	static void SortTypesByDesc()
	{
		std::sort(get_i2t().begin(), get_i2t().end(), &CompareTypes);
	}

public:
	bool GetEditorMode() const { return _modeEditor; }
	void SetEditorMode(bool editorModeEnable);
	GC_Object* CreateObject(ObjectType type, float x, float y);
	GC_2dSprite* PickEdObject(const vec2d &pt, int layer);

	static int GetTypeCount()
	{
		return get_i2t().size();
	}
	static const EdItem& GetTypeInfoByIndex(int typeIndex)
	{
		return get_t2i()[get_i2t()[typeIndex]];
	}
	static const EdItem& GetTypeInfo(ObjectType type)
	{
		return get_t2i()[type];
	}
	static ObjectType GetTypeByIndex(int typeIndex)
	{
		return get_i2t()[typeIndex];
	}
	static ObjectType GetTypeByName(const char *name)
	{
		name2type::const_iterator it = get_n2t().find(name);
		return get_n2t().end() != it ? it->second : INVALID_OBJECT_TYPE;
	}
	static const char* GetTypeName(ObjectType type)
	{
		assert(IsRegistered(type));
		return get_t2i()[type].name;
	}
	template<class T>
	static void RegisterActor( const char *name, const char *desc, int layer, float width,
	                           float height, float align, float offset )
	{
		assert( !IsRegistered(T::GetTypeStatic()) );
		assert( 0 == get_n2t().count(name) );
		EdItem ei;
		ei.desc    = desc;  // index in localization table
		ei.name    = name;
		ei.layer   = layer;
		ei.size    = vec2d(width, height);
		ei.align   = align;
		ei.offset  = offset;
		ei.service = false;
		ei.Create  = CreateActor<T>;
		get_t2i()[T::GetTypeStatic()] = ei;
		get_n2t()[name] = T::GetTypeStatic();
		get_i2t().push_back(T::GetTypeStatic());
	//	SortTypesByDesc();
	}
	template<class T>
	static void RegisterService( const char *name, const char *desc )
	{
		assert( !IsRegistered(T::GetTypeStatic()) );
		assert( 0 == get_n2t().count(name) );
		EdItem ei = {0};
		ei.desc    = desc;
		ei.name    = name;
		ei.service = true;
		ei.Create  = CreateService<T>;
		get_t2i()[T::GetTypeStatic()] = ei;
		get_n2t()[name] = T::GetTypeStatic();
		get_i2t().push_back(T::GetTypeStatic());
	//	SortTypesByDesc();
	}
	static bool IsRegistered(ObjectType type)
	{
		return (get_t2i().find(type) != get_t2i().end());
	}
#pragma endregion

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
#ifndef _DEBUG
	{} // do nothing in release mode
#endif
		;
};

///////////////////////////////////////////////////////////////////////////////


template<class SelectorType>
void Level::RayTrace(Grid<ObjectList> &list, SelectorType &s) const
{
	//
	// overlap line
	//

	vec2d begin(s.GetCenter() - s.GetDirection()/2), end(s.GetCenter() + s.GetDirection()/2), delta(s.GetDirection());
	begin /= LOCATION_SIZE;
	end   /= LOCATION_SIZE;
	delta /= LOCATION_SIZE;

	const int halfBeginX = int(floor(begin.x - 0.5f));
	const int halfBeginY = int(floor(begin.y - 0.5f));

	const int halfEndX = int(floor(end.x - 0.5f));
	const int halfEndY = int(floor(end.y - 0.5f));

	static const int jitX[4] = {0,1,0,1};
	static const int jitY[4] = {0,0,1,1};

	const int stepx = delta.x > 0 ? 2 : -2;
	const int stepy = delta.y > 0 ? 2 : -2;

	const float p = delta.y * (begin.x - 0.5f) - delta.x * (begin.y - 0.5f);
	const float tx = p - delta.y * (float) stepx;
	const float ty = p + delta.x * (float) stepy;

	for( int i = 0; i < 4; i++ )
	{
		int cx = halfBeginX + jitX[i];
		int cy = halfBeginY + jitY[i];

		int count = (abs(cx-halfEndX - (cx<halfEndX))>>1) + (abs(cy-halfEndY - (cy<halfEndY))>>1);
		assert(count >= 0);

		do
		{
			// check current cell
			if( cx >= 0 && cx < _locationsX && cy >= 0 && cy < _locationsY )
			{
				const ObjectList &tmp_list = list.element(cx, cy);
				for( ObjectList::iterator it = tmp_list.begin(); it != tmp_list.end(); ++it )
				{
					GC_RigidBodyStatic *object = static_cast<GC_RigidBodyStatic *>(*it);
					if( object->CheckFlags(GC_FLAG_RBSTATIC_PHANTOM|GC_FLAG_RBSTATIC_TRACE0) )
					{
						continue;
					}

					float hitEnter, hitExit;
					vec2d hitNorm;
					if( object->CollideWithLine(s.GetCenter(), s.GetDirection(), hitNorm, hitEnter, hitExit) )
					{
						assert(!_isnan(hitEnter) && _finite(hitEnter));
						assert(!_isnan(hitExit) && _finite(hitExit));
						assert(!_isnan(hitNorm.x) && _finite(hitNorm.x));
						assert(!_isnan(hitNorm.y) && _finite(hitNorm.y));
#ifndef NDEBUG
						for( int i = 0; i < 4; ++i )
						{
							g_level->DbgLine(object->GetVertex(i), object->GetVertex((i+1)&3));
						}
#endif
						if( s.Select(object, hitNorm, hitEnter, hitExit) )
						{
							return;
						}
					}
				}
			}

			// step to the next cell
			float t = delta.x * (float) cy - delta.y * (float) cx;
			if( fabs(t + tx) < fabs(t + ty) )
				cx += stepx;
			else
				cy += stepy;
		} while( count-- );
	}
}

///////////////////////////////////////////////////////////////////////////////
// end of file
