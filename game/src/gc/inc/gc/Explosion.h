#pragma once
#include "Actor.h"
#include "ObjPtr.h"
#include "WorldCfg.h"
#include <map>

class GC_Player;

class GC_Explosion : public GC_Actor
{
	struct FieldNode
	{
		FieldNode *parent;

		// horizontal size = 12; diagonal = 17
		short x;
		short y;
		unsigned int distance : 30;
		bool checked          :  1;
		bool open             :  1;

		FieldNode()
		{
			distance = 0;
			checked  = false;
			open     = true;
			parent   = nullptr;
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


	float CheckDamage(FIELD_TYPE &field, float dst_x, float dst_y, float max_distance);
	void Boom(World &world, float radius, float damage);

	ObjPtr<GC_Player> _owner;
	float _damage;
	float _radius;

public:
	GC_Explosion(vec2d pos);
	GC_Explosion(FromFile);
	virtual ~GC_Explosion();

	void SetDamage(float damage) { _damage = damage; }
	void SetOwner(GC_Player *owner) { _owner = owner; }
	void SetRadius(float radius);
	void SetTimeout(World &world, float timeout);

	// GC_Object
	void Resume(World &world) override;
	void Serialize(World &world, SaveFile &f) override;
};

class GC_ExplosionBig : public GC_Explosion
{
	DECLARE_SELF_REGISTRATION(GC_ExplosionBig);
public:
	GC_ExplosionBig(vec2d pos);
	GC_ExplosionBig(FromFile);

	// GC_Object
	void Init(World &world) override;
};

class GC_ExplosionStandard : public GC_Explosion
{
	DECLARE_SELF_REGISTRATION(GC_ExplosionStandard);
public:
	GC_ExplosionStandard(vec2d pos);
	GC_ExplosionStandard(FromFile);

	// GC_Object
	void Init(World &world) override;
};
