// Level.h

#pragma once

#include "gc/Object.h" // FIXME!


#pragma region path finding stuff

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
	float _pathBefore;          // стоимость пути до данной клетки
	float _pathAfter;           // оценка стоимости пути от данной клетки до пункта назначения

	inline int GetObjectsCount() const { return _objCount; }
	inline GC_RigidBodyStatic* GetObject(int index) const { return _ppObjects[index]; }

	inline bool IsChecked() const { return _mySession == _sessionId; }
	inline void Check()           { _mySession = _sessionId;         }

	inline void UpdatePath(short x, short y)
	{
		int dx = abs(x - GetX());
		int dy = abs(y - GetY());
		_pathAfter = (float) __max(dx, dy) + (float) __min(dx, dy) * 0.4142f;
	}

	void AddObject(GC_RigidBodyStatic *object);
	void RemoveObject(GC_RigidBodyStatic *object);

	inline short GetX()               const { return _x;    }
	inline short GetY()               const { return _y;    }
	inline unsigned char Properties() const { return _prop; }

	inline float Rate() const { return _pathBefore + _pathAfter; }

	inline bool operator > (const FieldCell &cell) const
	{
		_ASSERT(_mySession == cell._mySession);
		return Rate() > cell.Rate();
	}
};

class RefFieldCell
{
	FieldCell *_cell;
public:
	RefFieldCell(FieldCell &cell) { _cell = &cell; }
	operator FieldCell& () const { return *_cell; }
	bool operator > (const RefFieldCell &cell) const
	{
		return *_cell > *cell._cell;
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
class GC_Sound;
class GC_Wood;
class GC_RigidBodyStatic;
class GC_Wall;
class GC_Pickup;
class GC_2dSprite;
class GC_Background;
class GC_Text;


//////////////////////

class TankServer;
class TankClient;

class Level
{
#ifdef _DEBUG
	BOOL _bInitialized;
#endif

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

	OBJECT_LIST _objectLists[GLOBAL_LIST_COUNT];

public:

#ifdef NETWORK_DEBUG
	DWORD _checksum;
#endif

	OBJECT_LIST& GetList(GlobalListID id) { return _objectLists[id]; }
	const OBJECT_LIST& GetList(GlobalListID id) const { return _objectLists[id]; }

	OBJECT_GRIDSET  grid_rigid_s;
	OBJECT_GRIDSET  grid_walls;
	OBJECT_GRIDSET  grid_wood;
	OBJECT_GRIDSET  grid_water;
	OBJECT_GRIDSET  grid_pickup;

	OBJECT_LIST     ts_floating;
	OBJECT_LIST     ts_fixed;
	OBJECT_LIST     endframe;

	// graphics
	OBJECT_LIST     z_globals[Z_COUNT];
	OBJECT_GRIDSET  z_grids[Z_COUNT];

	SafePtr<GC_Background> _background;
	SafePtr<GC_Text>   _temporaryText;
	void DrawText(const char *string, const vec2d &position, enumAlignText align = alignTextLT);

/////////////////////////////////////
//settings
	bool    _freezed;
	bool    _modeEditor;
	bool    _limitHit;  // достигнут fraglimit или timelimit
	float   _sx, _sy;   // размер уровня

	int _locations_x;
	int _locations_y;

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
	int _pause;
	float _time;
	float _timeBuffer;

	Field _field;

	bool  _safeMode;

/////////////////////////////////////////////////////
	Level();
	~Level();

	void Init(int X, int Y);
	void Resize(int X, int Y);

	void HitLimit();


	bool init_emptymap();
	bool init_import_and_edit(const char *mapName);

	bool init_newdm(const char *mapName, unsigned long seed);
	bool init_load(const char *fileName);


public:
	bool RestoreObject(ObjectType otType, HANDLE file);

	bool Unserialize(const char *fileName);
	bool Serialize(const char *fileName);

	bool Export(const char *fileName);
	bool Import(const char *fileName, bool execInitScript);

	void PauseLocal(bool pause);
	void PauseSound(bool pause);
	void Freeze(bool freeze) { _freezed = freeze; }

	void TimeStep(float dt);
	void Render() const;
	bool IsSafeMode() const { return _safeMode; }

	GC_Object* FindObject(const char *name) const;

	int   net_rand();
	float net_frand(float max);
	vec2d net_vrand(float len);

	void CalcOutstrip( const vec2d &fp,    // fire point
                       float vp,           // speed of the projectile
                       const vec2d &tx,    // target position
                       const vec2d &tv,    // target velocity
                       vec2d &out_fake );  // out: fake target position

	void LocationFromPoint(const vec2d &pt, Location &l);

	//
	// tracing
	//

public:
	GC_RigidBodyStatic* agTrace( GridSet<OBJECT_LIST> &list,
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
	GC_2dSprite* PickEdObject(const vec2d &pt);

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
		_ASSERT(IsRegistered(type));
		return get_t2i()[type].name;
	}
	template<class T>
	static void RegisterActor( const char *name, const char *desc, int layer, float width,
	                           float height, float align, float offset )
	{
		_ASSERT( !IsRegistered(T::GetTypeStatic()) );
		EdItem ei;
		ei.desc    = desc;
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
		SortTypesByDesc();
	}
	template<class T>
	static void RegisterService( const char *name, const char *desc )
	{
		_ASSERT( !IsRegistered(T::GetTypeStatic()) );
		EdItem ei = {0};
		ei.desc    = desc;
		ei.name    = name;
		ei.service = true;
		ei.Create  = CreateService<T>;
		get_t2i()[T::GetTypeStatic()] = ei;
		get_n2t()[name] = T::GetTypeStatic();
		get_i2t().push_back(T::GetTypeStatic());
		SortTypesByDesc();
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

};

///////////////////////////////////////////////////////////////////////////////
// end of file
