// Actor.cpp

#include "stdafx.h"

#include "Actor.h"

#include "fs/MapFile.h"
#include "fs/SaveFile.h"

#include "Level.h"

///////////////////////////////////////////////////////////////////////////////

GC_Actor::GC_Actor() : GC_Object()
{
	ZeroMemory(&_location, sizeof(Location));
	MoveTo(vec2d(0, 0));
}

GC_Actor::GC_Actor(FromFile)
{
}

GC_Actor::~GC_Actor()
{
}

void GC_Actor::Kill()
{
	LeaveAllContexts();
	GC_Object::Kill();
}

void GC_Actor::Serialize(SaveFile &f)
{
	GC_Object::Serialize(f);

	f.Serialize(_location);
	f.Serialize(_pos);
}

void GC_Actor::MoveTo(const vec2d &pos)
{
	Location l;
	g_level->LocationFromPoint(pos, l);

	_pos = pos;

	if( 0 != memcmp(&l, &_location, sizeof(Location)) )
	{
		LeaveAllContexts();
		EnterAllContexts(l);
	}

	PulseNotify(NOTIFY_ACTOR_MOVE);
}

void GC_Actor::LeaveAllContexts()
{
	for( CONTEXTS_ITERATOR it = _contexts.begin(); it != _contexts.end(); ++it )
		LeaveContext(*it);
}

void GC_Actor::LeaveContext(Context &context)
{
	_ASSERT(context.inContext);
	(*context.grids)(_location.level).
		element(_location.x, _location.y).safe_erase(context.iterator);
	context.inContext = FALSE;
}

void GC_Actor::EnterAllContexts(const Location &l)
{
	_ASSERT(!IsKilled());
	_location = l;
	for( CONTEXTS_ITERATOR it = _contexts.begin(); it != _contexts.end(); ++it )
		EnterContext(*it, _location);
}

void GC_Actor::EnterContext(Context &context, const Location &l)
{
	_ASSERT(!IsKilled());
	_ASSERT(!context.inContext);

	(*context.grids)(l.level).element(l.x, l.y).push_front(this);
	context.iterator  = (*context.grids)(l.level).element(l.x, l.y).begin();
	context.inContext = true;
}

void GC_Actor::AddContext(OBJECT_GRIDSET *pGridSet)
{
	_ASSERT(!IsKilled());

	Context context;
	context.inContext  = false;
	context.grids      = pGridSet;

	_contexts.push_front(context);
	EnterContext(_contexts.front(), _location);
}

void GC_Actor::RemoveContext(OBJECT_GRIDSET *pGridSet)
{
	for( CONTEXTS_ITERATOR it = _contexts.begin(); it != _contexts.end(); ++it )
	{
		if( it->grids == pGridSet )
		{
			if( it->inContext)
				LeaveContext(*it);
			_contexts.erase(it);
			return;
		}
	}
	// не найден удаляемый контекст
	_ASSERT(FALSE);
}

//SafePtr<PropertySet> GC_Actor::GetProperties()
//{
//	return new MyPropertySet(this);
//}

void GC_Actor::mapExchange(MapFile &f)
{
	GC_Object::mapExchange(f);

	if( !f.loading() )
	{
		// координаты только сохраняются.
		// загруженные значения передаются через конструктор.
		MAP_EXCHANGE_FLOAT(x, _pos.x, 0);
		MAP_EXCHANGE_FLOAT(y, _pos.y, 0);
	}
}

///////////////////////////////////////////////////////////////////////////////
// end of file
