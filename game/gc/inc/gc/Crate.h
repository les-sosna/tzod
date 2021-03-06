#pragma once
#include "RigidBodyDynamic.h"

class GC_Crate : public GC_RigidBodyDynamic
{
	DECLARE_SELF_REGISTRATION(GC_Crate);

public:
	explicit GC_Crate(vec2d pos);
	explicit GC_Crate(FromFile);
	~GC_Crate();

	void OnDestroy(World &world, const DamageDesc &dd) override;

	float GetDefaultHealth() const override { return 50; }
	uint8_t GetObstacleFlags() const override { return 1; }
};
