// Actor.h

#pragma once

#include "Object.h"

///////////////////////////////////////////////////////////////////////////////

#define GC_FLAG_ACTOR_          GC_FLAG_OBJECT_

///////////////////////////////////////////////////////////////////////////////

// forward declarations
class GC_Pickup;


class GC_Actor : public GC_Object
{
	struct Context
	{
		Grid<OBJECT_LIST> *grids;
		OBJECT_LIST::iterator iterator;
	};

	typedef std::list<Context>::iterator CONTEXTS_ITERATOR;

	std::list<Context> _contexts;    // список контекстов данного объекта
	Location           _location;    // координаты в контексте.

	vec2d _pos;

	void LeaveAllContexts();
	void EnterAllContexts(const Location &l);
	void EnterContext(Context &context, const Location &l);
	void LeaveContext(Context &context);

protected:
	virtual void Serialize(SaveFile &f);
	virtual void mapExchange(MapFile &f);

	void AddContext(OBJECT_GRIDSET *pGridSet);
	void RemoveContext(OBJECT_GRIDSET *pGridSet);

public:
	const vec2d& GetPos() const { return _pos; }

	GC_Actor();
	GC_Actor(FromFile);
	virtual ~GC_Actor();

	virtual void Kill();
	virtual void MoveTo(const vec2d &pos);

	virtual void OnPickup(GC_Pickup *pickup, bool attached); // called by the pickup
};

///////////////////////////////////////////////////////////////////////////////
// end of file

