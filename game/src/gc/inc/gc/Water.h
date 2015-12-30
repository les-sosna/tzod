#pragma once
#include "RigidBody.h"
#include "NeighborAware.h"

#define GC_FLAG_WATER_              (GC_FLAG_RBSTATIC_ << 0)

class GC_Water : public GC_RigidBodyStatic
               , public GI_NeighborAware
{
	DECLARE_SELF_REGISTRATION(GC_Water);
    typedef GC_RigidBodyStatic base;

public:
	GC_Water(vec2d pos);
	GC_Water(FromFile);
	~GC_Water();

	// GC_Object
	void Init(World &world) override;
	void Kill(World &world) override;

	// GC_Actor
	void MoveTo(World &world, const vec2d &pos) override;

	// GI_NeighborAware
	int GetNeighbors(const World &world) const override;

	// GC_RigidBodyStatic
	unsigned char GetPassability() const override { return 0xFF; }  // impassable
	float GetDefaultHealth() const override { return 0; }

protected:
	void OnDamage(World &world, DamageDesc &dd) override;
};

