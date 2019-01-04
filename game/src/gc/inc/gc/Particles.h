#pragma once
#include "Decal.h"

class GC_BrickFragment : public GC_MovingObject
{
	DECLARE_SELF_REGISTRATION(GC_BrickFragment);
	DECLARE_LIST_MEMBER(override);

public:
	GC_BrickFragment(vec2d pos, vec2d v0);
	GC_BrickFragment(FromFile);

	void Serialize(World &world, SaveFile &f) override;
	void TimeStep(World &world, float dt) override;

private:
	int _startFrame;

	float _time;
	float _timeLife;

	vec2d _velocity;
};

///////////////////////////////////////////////////////////////////////////////

#define SPEED_SMOKE vec2d{0, -40.0f}

class GC_Particle : public GC_Decal
{
	DECLARE_SELF_REGISTRATION(GC_Particle);
	DECLARE_LIST_MEMBER(override);

public:
	GC_Particle(vec2d pos, vec2d v, DecalType ptype, float lifeTime, float age = 0);
	GC_Particle(FromFile);

	// GC_Object
	void Serialize(World &world, SaveFile &f) override;
	void TimeStep(World &world, float dt) override;

private:
	vec2d _velocity;
};
