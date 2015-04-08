#pragma once
#include "Object.h"
#include <math/MyMath.h>

#define GC_FLAG_ACTOR_INGRIDSET     (GC_FLAG_OBJECT_ << 0)
#define GC_FLAG_ACTOR_              (GC_FLAG_OBJECT_ << 1)

class GC_Actor : public GC_Object
{
public:
	explicit GC_Actor(vec2d pos);
	explicit GC_Actor(FromFile) {}
	
	const vec2d& GetDirection() const { return _direction; }
	void SetDirection(const vec2d &d) { assert(fabs(d.sqr()-1)<1e-5); _direction = d; }
	
	void SetGridSet(bool bGridSet) { SetFlags(GC_FLAG_ACTOR_INGRIDSET, bGridSet); }
	bool GetGridSet() const { return CheckFlags(GC_FLAG_ACTOR_INGRIDSET); }

	const vec2d& GetPos() const { return _pos; }
	virtual void MoveTo(World &world, const vec2d &pos);

	// GC_Object
	virtual void Init(World &world);
    virtual void Kill(World &world);
	virtual void MapExchange(MapFile &f);
	virtual void Serialize(World &world, SaveFile &f);
	
protected:
	int _locationX;
	int _locationY;
	virtual void EnterContexts(World &);
	virtual void LeaveContexts(World &);

private:
	vec2d _pos;
	vec2d _direction;
};


#define DECLARE_GRID_MEMBER()                                               \
protected:                                                                  \
    virtual void EnterContexts(World &world) override;                      \
    virtual void LeaveContexts(World &world) override;                      \
private:

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

