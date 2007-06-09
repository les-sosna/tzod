// Actor.h

#pragma once

#include "Object.h"

class GC_Actor : public GC_Object
{
	struct Context
	{
		GridSet<OBJECT_LIST> *grids;
		//-------
		OBJECT_LIST::iterator iterator;
		bool inContext;
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
};

///////////////////////////////////////////////////////////////////////////////
// end of file

