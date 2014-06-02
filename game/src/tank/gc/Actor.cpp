// Actor.cpp

#include "Actor.h"

#include "World.h"
#include "MapFile.h"
#include "SaveFile.h"

///////////////////////////////////////////////////////////////////////////////

GC_Actor::GC_Actor()
{
	memset(&_location, 0, sizeof(Location));
}

GC_Actor::~GC_Actor()
{
	LeaveAllContexts();
}

void GC_Actor::Serialize(World &world, SaveFile &f)
{
	GC_Object::Serialize(world, f);
	f.Serialize(_location);
	f.Serialize(_pos);
}

void GC_Actor::MoveTo(World &world, const vec2d &pos)
{
	Location loc;
	loc.x = std::min(world._locationsX-1, std::max(0, int(pos.x / LOCATION_SIZE)));
	loc.y = std::min(world._locationsY-1, std::max(0, int(pos.y / LOCATION_SIZE)));

	_pos = pos;

	if( 0 != memcmp(&loc, &_location, sizeof(Location)) )
	{
		LeaveAllContexts();
		EnterAllContexts(loc);
	}

	PulseNotify(world, NOTIFY_ACTOR_MOVE);
}

void GC_Actor::OnPickup(World &world, GC_Pickup *pickup, bool attached)
{
}

void GC_Actor::LeaveAllContexts()
{
	for( CONTEXTS_ITERATOR it = _contexts.begin(); it != _contexts.end(); ++it )
	{
		LeaveContext(*it);
	}
}

void GC_Actor::LeaveContext(Context &context)
{
	context.grids->element(_location.x,_location.y).erase(context.iterator);
}

void GC_Actor::EnterAllContexts(const Location &l)
{
	_location = l;
	for( CONTEXTS_ITERATOR it = _contexts.begin(); it != _contexts.end(); ++it )
	{
		EnterContext(*it, _location);
	}
}

void GC_Actor::EnterContext(Context &context, const Location &l)
{
	context.iterator = context.grids->element(l.x, l.y).insert(this);
}

void GC_Actor::AddContext(Grid<ObjectList> *pGridSet)
{
	Context context;
	context.grids = pGridSet;

	_contexts.push_front(context);
	EnterContext(_contexts.front(), _location);
}

void GC_Actor::RemoveContext(Grid<ObjectList> *pGridSet)
{
	for( CONTEXTS_ITERATOR it = _contexts.begin(); it != _contexts.end(); ++it )
	{
		if( it->grids == pGridSet )
		{
            LeaveContext(*it);
			_contexts.erase(it);
			return;
		}
	}
	// context not found
	assert(false);
}

void GC_Actor::MapExchange(World &world, MapFile &f)
{
	GC_Object::MapExchange(world, f);

	if( !f.loading() )
	{
		// we only store coordinates.
		// loaded coordinates are being passed through the constructor.
		MAP_EXCHANGE_FLOAT(x, _pos.x, 0);
		MAP_EXCHANGE_FLOAT(y, _pos.y, 0);
	}
}

///////////////////////////////////////////////////////////////////////////////
// end of file
