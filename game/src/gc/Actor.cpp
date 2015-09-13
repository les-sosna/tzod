#include "inc/gc/Actor.h"
#include "inc/gc/World.h"
#include "inc/gc/WorldCfg.h"
#include "inc/gc/SaveFile.h"
#include <MapFile.h>

// Workaround for IMPLEMENT_GRID_MEMBER macro used in the base class
namespace base
{
	inline static void EnterContexts(World&){}
	inline static void LeaveContexts(World&){}
}

IMPLEMENT_GRID_MEMBER(GC_Actor, grid_actors)

GC_Actor::GC_Actor(vec2d pos)
	: _pos(pos)
	, _direction(1, 0)
{
	SetFlags(GC_FLAG_ACTOR_INGRIDSET, true);
}

void GC_Actor::Init(World &world)
{
	GC_Object::Init(world);
	_locationX = std::min(world._locationsX-1, std::max(0, int(_pos.x / LOCATION_SIZE)));
	_locationY = std::min(world._locationsY-1, std::max(0, int(_pos.y / LOCATION_SIZE)));
	EnterContexts(world);
}

void GC_Actor::Kill(World &world)
{
	LeaveContexts(world);
	GC_Object::Kill(world);
}

void GC_Actor::Serialize(World &world, SaveFile &f)
{
	GC_Object::Serialize(world, f);
	f.Serialize(_locationX);
	f.Serialize(_locationY);
	f.Serialize(_pos);
	f.Serialize(_direction);

    if (f.loading())
        EnterContexts(world);
}

void GC_Actor::MoveTo(World &world, const vec2d &pos)
{
	_pos = pos;

	int locX = std::min(world._locationsX-1, std::max(0, int(pos.x / LOCATION_SIZE)));
	int locY = std::min(world._locationsY-1, std::max(0, int(pos.y / LOCATION_SIZE)));

    bool locationChanged = _locationX != locX || _locationY != locY;

	if( locationChanged )
        LeaveContexts(world);

    if( locationChanged )
    {
        _locationX = locX;
        _locationY = locY;
        EnterContexts(world);
 	}
}

void GC_Actor::MapExchange(MapFile &f)
{
	GC_Object::MapExchange(f);

	if( !f.loading() )
	{
		// we only store coordinates.
		// loaded coordinates are being passed through the constructor.
		MAP_EXCHANGE_FLOAT(x, _pos.x, 0);
		MAP_EXCHANGE_FLOAT(y, _pos.y, 0);
	}
}
