#include "inc/gc/Actor.h"
#include "inc/gc/World.h"
#include "inc/gc/WorldCfg.h"
#include "inc/gc/SaveFile.h"
#include <MapFile.h>

// Workaround for IMPLEMENT_GRID_MEMBER macro used in the base class
namespace base
{
	inline static void EnterContexts(World&, int, int){}
	inline static void LeaveContexts(World&, int, int){}
}

IMPLEMENT_GRID_MEMBER(GC_Actor, grid_actors)

GC_Actor::GC_Actor(vec2d pos)
	: _pos(pos)
	, _direction{ 1, 0 }
{
	SetFlags(GC_FLAG_ACTOR_INGRIDSET, true);
}

void GC_Actor::Init(World &world)
{
	GC_Object::Init(world);
	_locationX = std::max(world._locationBounds.left, std::min((int)std::floor(_pos.x / WORLD_LOCATION_SIZE), world._locationBounds.right - 1));
	_locationY = std::max(world._locationBounds.top, std::min((int)std::floor(_pos.y / WORLD_LOCATION_SIZE), world._locationBounds.bottom - 1));
	EnterContexts(world, _locationX, _locationY);
}

void GC_Actor::Kill(World &world)
{
	LeaveContexts(world, _locationX, _locationY);
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
		EnterContexts(world, _locationX, _locationY);
}

void GC_Actor::MoveTo(World &world, const vec2d &pos)
{
	_pos = pos;

	int locX = std::max(world._locationBounds.left, std::min((int)std::floor(_pos.x / WORLD_LOCATION_SIZE), world._locationBounds.right - 1));
	int locY = std::max(world._locationBounds.top, std::min((int)std::floor(_pos.y / WORLD_LOCATION_SIZE), world._locationBounds.bottom - 1));

	if (_locationX != locX || _locationY != locY)
	{
		LeaveContexts(world, _locationX, _locationY);
		EnterContexts(world, locX, locY);
		_locationX = locX;
		_locationY = locY;
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
