#pragma once

#include "Actor.h"
#include "constants.h"

class GC_Player;
class GC_Light;

class GC_Explosion : public GC_Actor
{
	DECLARE_SELF_REGISTRATION(GC_Explosion);
    typedef GC_Actor base;
	
	struct FieldNode
	{
		FieldNode *parent;
		
		// horizontal size = 12; diagonal = 17
		unsigned int x        : 10;
		unsigned int y        : 10;
		unsigned int distance : 10;
		bool checked          :  1;
		bool open             :  1;
		
		FieldNode()
		{
			distance = 0;
			checked  = false;
			open     = true;
			parent   = NULL;
		};
		
		float GetRealDistance() const
		{
			return (float) distance / 12.0f * (float) CELL_SIZE;
		}
	};
	
	struct coord
	{
		short x, y;
		coord() {}
		coord(short x_, short y_) { x = x_; y = y_; }
		operator size_t () const { return x + LEVEL_MAXSIZE * y; }
	};
	
	typedef std::map<coord, FieldNode> FIELD_TYPE;
	
	bool _boomOK;
	
	ObjPtr<GC_Player>  _owner;
	ObjPtr<GC_Light>   _light;
	
	float CheckDamage(FIELD_TYPE &field, float dst_x, float dst_y, float max_distance);
	void Boom(World &world, float radius, float damage);
	
	float _damage;
	float _radius;
	float _time;
	float _time_life;
	float _time_boom;
	
public:
    DECLARE_LIST_MEMBER();
	GC_Explosion(World &world, GC_Player *owner, float duration);
	GC_Explosion(FromFile);
	virtual ~GC_Explosion();
	
	void SetRadius(float radius);
	void SetDamage(float damage) { _damage = damage; }
	void SetBoomTimeout(float t) { _time_boom = t; }

	// GC_Actor
	virtual void MoveTo(World &world, const vec2d &pos) override;
	
	// GC_Object
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStep(World &world, float dt);
    virtual void Kill(World &world) override;
};

GC_Explosion& MakeExplosionStandard(World &world, const vec2d &pos, GC_Player *owner);
GC_Explosion& MakeExplosionBig(World &world, const vec2d &pos, GC_Player *owner);
