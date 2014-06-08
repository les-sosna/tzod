// Actor.cpp

#include "Actor.h"
#include "World.h"

#include "constants.h"
#include "MapFile.h"
#include "SaveFile.h"

///////////////////////////////////////////////////////////////////////////////

void GC_Actor::Serialize(World &world, SaveFile &f)
{
	GC_Object::Serialize(world, f);
	f.Serialize(_locationX);
	f.Serialize(_locationY);
	f.Serialize(_pos);
    
    if (f.loading() && CheckFlags(GC_FLAG_ACTOR_KNOWNPOS))
        EnterContexts(world);
}

void GC_Actor::MoveTo(World &world, const vec2d &pos)
{
	_pos = pos;
    
	int locX = std::min(world._locationsX-1, std::max(0, int(pos.x / LOCATION_SIZE)));
	int locY = std::min(world._locationsY-1, std::max(0, int(pos.y / LOCATION_SIZE)));
    
    bool locationChanged = _locationX != locX || _locationY != locY;

	if( locationChanged && CheckFlags(GC_FLAG_ACTOR_KNOWNPOS) )
        LeaveContexts(world);

    if( locationChanged || !CheckFlags(GC_FLAG_ACTOR_KNOWNPOS) )
    {
        _locationX = locX;
        _locationY = locY;
        EnterContexts(world);
        SetFlags(GC_FLAG_ACTOR_KNOWNPOS, true);
 	}

	PulseNotify(world, NOTIFY_ACTOR_MOVE);
}

void GC_Actor::OnPickup(World &world, GC_Pickup *pickup, bool attached)
{
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

void GC_Actor::Kill(World &world)
{
    if( CheckFlags(GC_FLAG_ACTOR_KNOWNPOS) )
        LeaveContexts(world);
    GC_Object::Kill(world);
}

///////////////////////////////////////////////////////////////////////////////
// end of file
