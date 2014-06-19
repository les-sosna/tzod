// Actor.h

#pragma once

#include "Object.h"
#include "core/Grid.h"
#include <list>


#define GC_FLAG_ACTOR_KNOWNPOS      (GC_FLAG_OBJECT_ << 0)
#define GC_FLAG_ACTOR_              (GC_FLAG_OBJECT_ << 1)


class GC_Pickup;
class World;

class GC_Actor : public GC_Object
{
	vec2d _pos;

protected:
	virtual void Serialize(World &world, ObjectList::id_type id, SaveFile &f);
	virtual void MapExchange(World &world, MapFile &f);
    
	int _locationX;
	int _locationY;
    virtual void EnterContexts(World &, ObjectList::id_type id) {}
    virtual void LeaveContexts(World &, ObjectList::id_type id) {}

public:
	const vec2d& GetPos() const { return _pos; }

	virtual void MoveTo(World &world, ObjectList::id_type id, const vec2d &pos);
	virtual void OnPickup(World &world, GC_Pickup *pickup, bool attached); // called by a pickup
    
    virtual void Kill(World &world, ObjectList::id_type id);
};


#define DECLARE_GRID_MEMBER()                                                   \
    virtual void EnterContexts(World &world, ObjectList::id_type id) override;  \
    virtual void LeaveContexts(World &world, ObjectList::id_type id) override;

#define IMPLEMENT_GRID_MEMBER(cls, grid)                                    \
    void cls::EnterContexts(World &world, ObjectList::id_type id)           \
    {                                                                       \
        base::EnterContexts(world, id);                                     \
        world.grid.element(_locationX, _locationY).push_back(id);           \
    }                                                                       \
    void cls::LeaveContexts(World &world, ObjectList::id_type id)           \
    {                                                                       \
        auto &cell = world.grid.element(_locationX,_locationY);             \
        auto where = std::find(cell.begin(), cell.end(), id);               \
        assert(where != cell.end());                                        \
        *where = cell.back();                                               \
        cell.pop_back();                                                    \
		base::LeaveContexts(world, id);                                     \
    }

// end of file

