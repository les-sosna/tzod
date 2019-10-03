#pragma once
#include "Grid.h"
#include "ObjPtr.h"
#include "WorldEvents.h"
#include "detail/GlobalListHelper.h"
#include "detail/JobManager.h"
#include "detail/MemoryManager.h"
#include "detail/PtrList.h"
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <vector>

namespace FS
{
	struct Stream;
}
class Field;
class MapFile;
class SaveFile;
class GC_Object;
class GC_Player;
class GC_RigidBodyStatic;

template<class> struct ObjectListener;

template<class T>
class EventsHub
{
public:
	void AddListener(ObjectListener<T> &ls)
	{
		_listeners.push_back(&ls);
	}

	void RemoveListener(ObjectListener<T> &ls)
	{
		auto it = std::find(_listeners.begin(), _listeners.end(), &ls);
		assert(_listeners.end() != it);
		*it = _listeners.back();
		_listeners.pop_back();
	}

private:
	friend T;
	std::vector<ObjectListener<T>*> _listeners;
};

#define DECLARE_EVENTS(cls) EventsHub<::cls> e##cls;

class ResumableObject
{
	DECLARE_POOLED_ALLOCATION(ResumableObject);
	ResumableObject(const ResumableObject&) = delete;
	ResumableObject& operator = (const ResumableObject&) = delete;
public:
	void Cancel() { ptr = nullptr; }

private:
	friend class World;
	explicit ResumableObject(GC_Object &obj) : ptr(&obj) {}
	ObjPtr<GC_Object> ptr;
};

#define SAFE_CANCEL(ro) if(ro) { ro->Cancel(); ro = nullptr; } else (void)0

class World
{
public:
	DECLARE_EVENTS(GC_Explosion);
	DECLARE_EVENTS(GC_Pickup);
	DECLARE_EVENTS(GC_pu_Shield);
	DECLARE_EVENTS(GC_Player);
	DECLARE_EVENTS(GC_Projectile);
	DECLARE_EVENTS(GC_ProjectileBasedWeapon);
	DECLARE_EVENTS(GC_RigidBodyStatic);
	DECLARE_EVENTS(GC_RigidBodyDynamic);
	DECLARE_EVENTS(GC_Trigger);
	DECLARE_EVENTS(GC_Turret);
	DECLARE_EVENTS(GC_Vehicle);
	DECLARE_EVENTS(World);

#ifdef NETWORK_DEBUG
	uint32_t _checksum;
	int _frame;
	FILE *_dump;
#endif

	static const unsigned int NET_RAND_MAX = 0xffff;

	PtrList<GC_Object>& GetList(GlobalListID id) { return _objectLists[id]; }
	const PtrList<GC_Object>& GetList(GlobalListID id) const { return _objectLists[id]; }

	JobManager<GC_Turret> _jobManager;

	Grid<PtrList<GC_Object>>  grid_rigid_s;
	Grid<PtrList<GC_Object>>  grid_walls;
	Grid<PtrList<GC_Object>>  grid_pickup;
	Grid<PtrList<GC_Object>>  grid_moving;

	std::vector<bool> _waterTiles;
	std::vector<bool> _woodTiles;

	std::string _infoAuthor;
	std::string _infoEmail;
	std::string _infoUrl;
	std::string _infoDesc;
	std::string _infoTheme;
	std::string _infoOnInit;

	unsigned long _seed;

	std::unique_ptr<Field> _field;
	bool  _safeMode;

public:
	World(RectRB blockBounds, bool initField);
	World(const World&) = delete;
	World& operator=(const World&) = delete;
	~World();

	void Step(float dt);


	template<class T, class ...Args>
	T& New(Args && ... args)
	{
		auto t = new T(std::forward<Args>(args)...);
		t->Register(*this);
		t->Init(*this);
		for( auto ls: eWorld._listeners )
			ls->OnNewObject(*t);
		return *t;
	}

	void Serialize(SaveFile &f);

	FRECT GetOccupiedBounds() const;
	void Export(FS::Stream &stream);
	void Import(MapFile &file);

	bool GetNightMode() const { return _nightMode; }
	bool IsSafeMode() const { return _safeMode; }
	GC_Object* FindObject(std::string_view name) const;

	int   net_rand();
	float net_frand(float max);
	vec2d net_vrand(float len);

	bool CalcOutstrip( vec2d origin,
	                   float projectileSpeed,
	                   vec2d targetPos,
	                   vec2d targetVelocity,
	                   vec2d &out_fake );  // out: fake target position

	int GetTileIndex(vec2d pos) const;
	int GetTileIndex(int blockX, int blockY) const;

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

	GC_RigidBodyStatic* TraceNearest( const Grid<PtrList<GC_Object>> &list,
	                             const GC_RigidBodyStatic* ignore,
	                             const vec2d &x0,      // origin
	                             const vec2d &a,       // direction and length
	                             vec2d *ht   = nullptr,
	                             vec2d *norm = nullptr) const;

	void TraceAll( const Grid<PtrList<GC_Object>> &list,
	               const vec2d &x0,      // origin
	               const vec2d &a,       // direction and length
	               std::vector<CollisionPoint> &result) const;

	template<class SelectorType>
	void RayTrace(const Grid<PtrList<GC_Object>> &list, SelectorType &s) const;

public:
	void Clear();
	GC_Player* GetPlayerByIndex(size_t playerIndex);
	void Seed(unsigned long seed);

	float GetTime() const { return _time; }
	const RectRB& GetLocationBounds() const { return _locationBounds; }
	const RectRB& GetBlockBounds() const { return _blockBounds; }
	const FRECT& GetBounds() const { return _bounds; }

#ifdef NETWORK_DEBUG
	uint32_t GetChecksum() const { return _checksum; }
	unsigned int GetFrame() const { return _frame; }
#endif

	ResumableObject* Timeout(GC_Object &obj, float timeout);
	size_t GetResumableCount() const { return _resumables.size(); }

private:
	struct Resumable
	{
		std::unique_ptr<ResumableObject> obj;
		float time;
		bool operator<(const Resumable &other) const
		{
			return time > other.time;
		}
		Resumable(std::unique_ptr<ResumableObject> obj_, float time_)
			: obj(std::move(obj_))
			, time(time_)
		{}
	};
	std::priority_queue<Resumable> _resumables;
	float _time;

	bool _gameStarted;
	bool _nightMode;
	FRECT _bounds;
	RectRB _blockBounds;
	RectRB _locationBounds;

	friend class GC_Object;

	void OnKill(GC_Object &obj);

	std::map<std::string, const GC_Object*, std::less<>> _nameToObjectMap;
	std::map<const GC_Object*, std::string_view> _objectToStringMap; // string owned by _nameToObjectMap

	PtrList<GC_Object> _objectLists[GLOBAL_LIST_COUNT];
};

