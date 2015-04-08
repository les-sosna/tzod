#pragma once
#include "RigidBodyDinamic.h"

class GC_Crate : public GC_RigidBodyDynamic
{
	DECLARE_SELF_REGISTRATION(GC_Crate);

public:
	explicit GC_Crate(vec2d pos);
	explicit GC_Crate(FromFile);
	~GC_Crate();

	virtual void OnDestroy(World &world, const DamageDesc &dd);

	virtual float GetDefaultHealth() const { return 50; }
	virtual unsigned char GetPassability() const { return 0; }
};
