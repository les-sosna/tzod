// Level.h

#pragma once

#include "ObjectListener.h"
#include "gc/Object.h" // FIXME!

#include "network/ControlPacket.h"

#include "video/RenderBase.h"


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
	unsigned char _prop;             // 0 - свободно, 1 - пробиваемо, 0xFF - непробиваемо
	//-----------------------------
	void UpdateProperties();
	FieldCell()
	{
		_ppObjects   = NULL;
		_objCount    = 0;
		_prop        = 0;     // свободно
		_mySession   = 0xffffffff;
	}
public:

	const FieldCell *_prevCell; // предыдущая клетка пути

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
	float _before;          // стоимость пути до данной клетки
	float _total;                // оценка общей стоимости пути
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


class Level : public RefCounted
{
	friend class GC_Object;

	std::map<const GC_Object*, string_t>  _objectToNameMap;
	std::map<string_t, const GC_Object*>  _nameToObjectMap;


	struct SaveHeader
	{
		DWORD dwVersion;
		DWORD dwGameType;
		bool  nightmode;
		float timelimit;
		int   fraglimit;
		int   nObjects;
		float time;
		int   width;
		int   height;
		char  theme[MAX_PATH];
	};

	ObjectList _objectLists[GLOBAL_LIST_COUNT];

public:

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

	ObjectList     ts_floating;
	ObjectList     ts_fixed;

	// graphics
	ObjectList        z_globals[Z_COUNT];
	Grid<ObjectList>  z_grids[Z_COUNT];

	ObjectListener *_serviceListener;

	size_t _texBack;
	size_t _texGrid;
	void DrawBackground(size_t tex) const;

/////////////////////////////////////
//settings
	bool    _frozen;
	bool    _modeEditor;
	bool    _limitHit;  // достигнут fraglimit или timelimit
	float   _sx, _sy;   // размер уровня

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
	int _pause;
	float _time;
	float _timeBuffer;

	void Step(const ControlPacketVector &ctrl, float dt);


	Field _field;

	int  _ctrlSentCount;
	bool  _safeMode;

/////////////////////////////////////////////////////
	Level();

	void Resize(int X, int Y);
	void Clear();

	void HitLimit();


	bool init_emptymap(int X, int Y);
	bool init_import_and_edit(const char *mapName);

	void init_newdm(const SafePtr<FS::Stream> &s, unsigned long seed);
	bool init_load(const char *fileName);


public:
	bool IsEmpty() const;

	bool RestoreObject(ObjectType otType, HANDLE file);

	bool Unserialize(const char *fileName);
	bool Serialize(const char *fileName);

	void Export(const SafePtr<FS::Stream> &file);
	void Import(const SafePtr<FS::Stream> &file, bool execInitScript);

	void PauseLocal(bool pause);
	void PauseSound(bool pause);
	void Freeze(bool freeze) { _frozen = freeze; }

	void RunCmdQueue(float dt);
	void TimeStep(float dt);
	void Render() const;
	bool IsSafeMode() const { return _safeMode; }

	GC_Object* FindObject(const string_t &name) const;

	int   net_rand();
	float net_frand(float max);
	vec2d net_vrand(float len);

	void CalcOutstrip( const vec2d &fp,    // fire point
                       float vp,           // speed of the projectile
                       const vec2d &tx,    // target position
                       const vec2d &tv,    // target velocity
                       vec2d &out_fake );  // out: fake target position


private:
	virtual ~Level();


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
	GC_RigidBodyStatic* agTrace( Grid<ObjectList> &list,
	                             const GC_RigidBodyStatic* ignore,
	                             const vec2d &x0,      // координаты начала
	                             const vec2d &a,       // направление
	                             vec2d *ht   = NULL,
	                             vec2d *norm = NULL) const;


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
	void ToggleEditorMode();
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
	void DbgLine(const vec2d &v1, const vec2d &v2, SpriteColor color = 0x00ff00ff);
};

///////////////////////////////////////////////////////////////////////////////
// end of file
