// World.h

#pragma once

#include "Field.h"

#include "GlobalListHelper.h"
#include "network/ControlPacket.h"
#include <core/PtrList.h>
#include <core/Grid.h>

#include <map>
#include <set>
#include <string>
#include <vector>
#include <memory>


namespace FS
{
	class Stream;
}
class ClientBase;
class GC_RigidBodyStatic;
class GC_Object;
class GC_Player;
class GC_2dSprite;

struct ObjectListener;
struct MessageListener;

class World
{
	friend class GC_Object;

	std::map<const GC_Object*, std::string>  _objectToStringMap;
	std::map<std::string, const GC_Object*>  _nameToObjectMap; // TODO: try to avoid name string duplication

	PtrList<GC_Object> _objectLists[GLOBAL_LIST_COUNT];

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
    Grid<PtrList<GC_Object>>  grid_sprites;

	ObjectListener *_serviceListener;
    MessageListener *_messageListener;

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

	void Step(float dt);

	Field _field;

	bool  _safeMode;

/////////////////////////////////////////////////////
	World();
	~World();
    
	void Resize(int X, int Y);
	void HitLimit();

	bool IsEmpty() const;

	void Unserialize(const char *fileName);
	void Serialize(const char *fileName);

	void Export(std::shared_ptr<FS::Stream> file);
	void Import(std::shared_ptr<FS::Stream> file);

	void PauseSound(bool pause);
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


	//
	// config callback handlers
	//

protected:
	void OnChangeSoundVolume();
	void OnChangeNightMode();

public:
	void Clear();
	GC_Player* GetPlayerByIndex(size_t playerIndex);
    void Seed(unsigned long seed);

	float GetTime() const { return _time; }

#ifdef NETWORK_DEBUG
    uint32_t GetChecksum() const { return _checksum; }
    unsigned int GetFrame() const { return _frame; }
#endif
};

// end of file
