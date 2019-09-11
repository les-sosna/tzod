#include "inc/gc/MovingObject.h"
#include "inc/gc/SaveFile.h"
#include "inc/gc/World.h"
#include "inc/gc/WorldCfg.h"
#include <MapFile.h>

// Workaround for IMPLEMENT_GRID_MEMBER macro used in the base class
namespace base
{
	inline static void EnterContexts(World&, int, int){}
	inline static void LeaveContexts(World&, int, int){}
}

IMPLEMENT_GRID_MEMBER(base, GC_MovingObject, grid_moving)

GC_MovingObject::GC_MovingObject(vec2d pos)
	: _pos(pos)
	, _direction{ 1, 0 }
{
	SetFlags(GC_FLAG_MO_INGRIDSET, true);
}

void GC_MovingObject::Init(World &world)
{
	_locationX = std::max(world.GetLocationBounds().left, std::min((int)std::floor(_pos.x / WORLD_LOCATION_SIZE), world.GetLocationBounds().right - 1));
	_locationY = std::max(world.GetLocationBounds().top, std::min((int)std::floor(_pos.y / WORLD_LOCATION_SIZE), world.GetLocationBounds().bottom - 1));
	EnterContexts(world, _locationX, _locationY);
}

void GC_MovingObject::Kill(World &world)
{
	LeaveContexts(world, _locationX, _locationY);
	GC_Object::Kill(world);
}

void GC_MovingObject::Serialize(World &world, SaveFile &f)
{
	GC_Object::Serialize(world, f);
	f.Serialize(_locationX);
	f.Serialize(_locationY);
	f.Serialize(_pos);
	f.Serialize(_direction);

	if (f.loading())
		EnterContexts(world, _locationX, _locationY);
}

void GC_MovingObject::MoveTo(World &world, const vec2d &pos)
{
	_pos = pos;

	int locX = std::max(world.GetLocationBounds().left, std::min((int)std::floor(_pos.x / WORLD_LOCATION_SIZE), world.GetLocationBounds().right - 1));
	int locY = std::max(world.GetLocationBounds().top, std::min((int)std::floor(_pos.y / WORLD_LOCATION_SIZE), world.GetLocationBounds().bottom - 1));

	if (_locationX != locX || _locationY != locY)
	{
		LeaveContexts(world, _locationX, _locationY);
		EnterContexts(world, locX, locY);
		_locationX = locX;
		_locationY = locY;
	}
}

void GC_MovingObject::MapExchange(MapFile &f)
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
