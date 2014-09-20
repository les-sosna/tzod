// Actor.h

#pragma once

#include "Object.h"
#include "core/Grid.h"
#include <list>


#define GC_FLAG_ACTOR_KNOWNPOS      (GC_FLAG_OBJECT_ << 0)
#define GC_FLAG_ACTOR_INGRIDSET     (GC_FLAG_OBJECT_ << 1)
#define GC_FLAG_ACTOR_              (GC_FLAG_OBJECT_ << 2)


class GC_Pickup;
class World;

class GC_Actor : public GC_Object
{
	vec2d _pos;
	vec2d _direction;

protected:
	virtual void Serialize(World &world, SaveFile &f);
	virtual void MapExchange(World &world, MapFile &f);
    
	int _locationX;
	int _locationY;
	virtual void EnterContexts(World &);
	virtual void LeaveContexts(World &);

public:
	GC_Actor();
	
	const vec2d& GetPos() const { return _pos; }

	const vec2d& GetDirection() const { return _direction; }
	void SetDirection(const vec2d &d) { assert(fabs(d.sqr()-1)<1e-5); _direction = d; }
	
	void SetGridSet(bool bGridSet) { SetFlags(GC_FLAG_ACTOR_INGRIDSET, bGridSet); }
	bool GetGridSet() const { return CheckFlags(GC_FLAG_ACTOR_INGRIDSET); }

	virtual void MoveTo(World &world, const vec2d &pos);
	virtual void OnPickup(World &world, GC_Pickup *pickup, bool attached); // called by a pickup
    
    virtual void Kill(World &world);
};


#define DECLARE_GRID_MEMBER()                                               \
    virtual void EnterContexts(World &world) override;                      \
    virtual void LeaveContexts(World &world) override;

#define IMPLEMENT_GRID_MEMBER(cls, grid)                                    \
    void cls::EnterContexts(World &world)                                   \
    {                                                                       \
        base::EnterContexts(world);                                         \
        world.grid.element(_locationX, _locationY).insert(this);            \
    }                                                                       \
    void cls::LeaveContexts(World &world)                                   \
    {                                                                       \
        auto &cell = world.grid.element(_locationX,_locationY);             \
        for (auto id = cell.begin(); id != cell.end(); id = cell.next(id))  \
        {                                                                   \
            if (cell.at(id) == this)                                        \
            {                                                               \
                cell.erase(id);                                             \
                base::LeaveContexts(world);                                 \
                return;                                                     \
            }                                                               \
        }                                                                   \
        assert(false);                                                      \
    }

// end of file

