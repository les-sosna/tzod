#pragma once
#include "RigidBody.h"
#include "NeighborAware.h"

#define GC_FLAG_WATER_INTILE        (GC_FLAG_RBSTATIC_ << 0)
#define GC_FLAG_WATER_              (GC_FLAG_RBSTATIC_ << 1)

class GC_Water : public GC_RigidBodyStatic
               , public GI_NeighborAware
{
	DECLARE_GRID_MEMBER();
	DECLARE_SELF_REGISTRATION(GC_Water);
    typedef GC_RigidBodyStatic base;

/**
 *   tile bits
 *
 *   5   6   7
 *    +-----+
 *    |     |
 *  4 |  #  | 0
 *    |     |
 *    +-----+
 *   3   2   1
**/
	int _tile;

protected:
	void UpdateTile(World &world, bool flag);

public:
	GC_Water(vec2d pos);
	GC_Water(FromFile);
	~GC_Water();

	void SetTile(char nTile, bool value);

	// GC_Object
	void Kill(World &world) override;
	void Serialize(World &world, SaveFile &f) override;

	// GC_Actor
	void MoveTo(World &world, const vec2d &pos) override;

	// GI_NeighborAware
	int GetNeighbors() const override { return _tile; }

	// GC_RigidBodyStatic
	unsigned char GetPassability() const override { return 0xFF; }  // impassable
	float GetDefaultHealth() const override { return 0; }

protected:
	void OnDamage(World &world, DamageDesc &dd) override;
};

