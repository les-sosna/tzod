// World.h

#pragma once

#include "Field.h"
#include "GlobalListHelper.h"
#include "ObjPtr.h"
#include "WorldEvents.h"
#include "MapFile.h"
#include <core/Grid.h>
#include <core/PtrList.h>
#include <core/MemoryManager.h>

#include <map>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <vector>


namespace FS
{
	class Stream;
}
class ClientBase;
class GC_Object;
class GC_Player;
class GC_RigidBodyStatic;

class ThemeManager; // todo: remove
class TextureManager; // todo: remove

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
	World(const World&) = delete;
	World& operator=(const World&) = delete;
	
	friend class GC_Object;
	
	void OnKill(GC_Object &obj);

	std::map<const GC_Object*, std::string>  _objectToStringMap;
	std::map<std::string, const GC_Object*>  _nameToObjectMap; // TODO: try to avoid name string duplication

	PtrList<GC_Object> _objectLists[GLOBAL_LIST_COUNT];

public:
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

#ifndef NDEBUG
	std::set<GC_Object*> _garbage;
#endif

#ifdef NETWORK_DEBUG
	uint32_t _checksum;
	int _frame;
	FILE *_dump;
#endif
	
	static const unsigned int NET_RAND_MAX = 0xffff;

	PtrList<GC_Object>& GetList(GlobalListID id) { return _objectLists[id]; }
	const PtrList<GC_Object>& GetList(GlobalListID id) const { return _objectLists[id]; }

	Grid<PtrList<GC_Object>>  grid_rigid_s;
	Grid<PtrList<GC_Object>>  grid_walls;
	Grid<PtrList<GC_Object>>  grid_wood;
	Grid<PtrList<GC_Object>>  grid_water;
	Grid<PtrList<GC_Object>>  grid_pickup;
    Grid<PtrList<GC_Object>>  grid_actors;

/////////////////////////////////////
	bool    _gameStarted;
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

	void Step(float dt);

	Field _field;

	bool  _safeMode;

/////////////////////////////////////////////////////
	World();
	~World();

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

	template<class T, class ...Args>
	T& RestoreObject(MapFile &file, Args && ... args)
	{
		auto t = new T(std::forward<Args>(args)...);
		t->MapExchange(file);
		std::string name;
		if (file.getObjectAttribute("name", name))
			t->SetName(*this, name.c_str());
		t->Register(*this);
		t->Init(*this);
		return *t;
	}
	
	void GameMessage(const char *msg);
	
	void Resize(int X, int Y);
	void HitLimit();

	bool IsEmpty() const;

	void Unserialize(std::shared_ptr<FS::Stream> stream, const ThemeManager &themeManager, TextureManager &tm);
	void Serialize(std::shared_ptr<FS::Stream> stream);

	void Export(std::shared_ptr<FS::Stream> file);
	void Import(std::shared_ptr<FS::Stream> file, const ThemeManager &themeManager, TextureManager &tm);

	void Freeze(bool freeze) { _frozen = freeze; }

	bool IsSafeMode() const { return _safeMode; }
	GC_Object* FindObject(const std::string &name) const;

	int   net_rand();
	float net_frand(float max);
	vec2d net_vrand(float len);

	bool CalcOutstrip( const vec2d &fp,    // fire point
                       float vp,           // speed of the projectile
                       const vec2d &tx,    // target position
                       const vec2d &tv,    // target velocity
                       vec2d &out_fake );  // out: fake target position


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

public:
	void Clear();
	GC_Player* GetPlayerByIndex(size_t playerIndex);
    void Seed(unsigned long seed);

	float GetTime() const { return _time; }

#ifdef NETWORK_DEBUG
    uint32_t GetChecksum() const { return _checksum; }
    unsigned int GetFrame() const { return _frame; }
#endif
	
	ResumableObject* Timeout(GC_Object &obj, float timeout);
	
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
#ifdef _MSC_VER
        Resumable(Resumable && other)
            : obj(std::move(other.obj))
            , time(other.time)
        {}
        void operator=(Resumable && other)
        {
            obj = std::move(other.obj);
            time = other.time;
        }
#endif
    };
	std::priority_queue<Resumable> _resumables;
	float _time;
};

// end of file
