#pragma once

#include "2dSprite.h"

class GC_Player;
class GC_Light;

class GC_Explosion : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Explosion);
    typedef GC_2dSprite base;
protected:
	
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
	
public:
	float _time;
	float _time_life;
	float _time_boom;
	
public:
    DECLARE_LIST_MEMBER();
	GC_Explosion(World &world, GC_Player *owner);
	GC_Explosion(FromFile);
	virtual ~GC_Explosion();
	
	
	float _damage;
	float _radius;
	
	void Boom(World &world, float radius, float damage);
	
	// GC_2dSprite
	virtual enumZOrder GetZ() const { return Z_EXPLODE; }
	
	// GC_Object
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStepFixed(World &world, float dt);
    virtual void Kill(World &world) override;
};

/////////////////////////////////////////////////////////////

class GC_Boom_Standard : public GC_Explosion
{
	DECLARE_SELF_REGISTRATION(GC_Boom_Standard);
public:
	GC_Boom_Standard(World &world, const vec2d &pos, GC_Player *owner);
	GC_Boom_Standard(FromFile);
	virtual ~GC_Boom_Standard();
};

/////////////////////////////////////////////////////////////

class GC_Boom_Big : public GC_Explosion
{
	DECLARE_SELF_REGISTRATION(GC_Boom_Big);
public:
	GC_Boom_Big(World &world, const vec2d &pos, GC_Player *owner);
	GC_Boom_Big(FromFile);
};
