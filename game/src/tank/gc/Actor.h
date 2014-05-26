// Actor.h

#pragma once

#include "Object.h"
#include "core/Grid.h"
#include <list>


#define GC_FLAG_ACTOR_          GC_FLAG_OBJECT_

class GC_Pickup;
class World;

class GC_Actor : public GC_Object
{
	struct Context
	{
		Grid<ObjectList> *grids;
		ObjectList::id_type iterator;
	};

	typedef std::list<Context>::iterator CONTEXTS_ITERATOR;

	std::list<Context> _contexts;    // contexts this object belongs to
	Location           _location;    // location on all contexts.

	vec2d _pos;

	void LeaveAllContexts();
	void EnterAllContexts(const Location &l);
	void EnterContext(Context &context, const Location &l);
	void LeaveContext(Context &context);

protected:
	virtual void Serialize(World &world, SaveFile &f);
	virtual void MapExchange(World &world, MapFile &f);

	void AddContext(Grid<ObjectList> *pGridSet);
	void RemoveContext(Grid<ObjectList> *pGridSet);

public:
	const vec2d& GetPos() const { return _pos; }

	GC_Actor(World &world);
	GC_Actor(FromFile);
	virtual ~GC_Actor();

	virtual void MoveTo(World &world, const vec2d &pos);

	virtual void OnPickup(World &world, GC_Pickup *pickup, bool attached); // called by the pickup
};

///////////////////////////////////////////////////////////////////////////////
// end of file

