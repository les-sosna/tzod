// particles.h

#pragma once

#include "2dSprite.h"

/////////////////////////////////////////////////////////////

class GC_Brick_Fragment_01 : public GC_2dSprite
{
	DECLARE_POOLED_ALLOCATION(GC_Brick_Fragment_01);
	DECLARE_SELF_REGISTRATION(GC_Brick_Fragment_01);

private:
	int _StartFrame;

	float _time;
	float _time_life;

	vec2d _velocity;

public:
	GC_Brick_Fragment_01(const vec2d &x0, const vec2d &v0);
	GC_Brick_Fragment_01(FromFile);

	virtual bool IsSaved() const { return true; }
	virtual void Serialize(SaveFile &f);

	virtual void TimeStepFloat(float dt);
};

/////////////////////////////////////////////////////////////

class GC_Particle : public GC_2dSprite
{
	DECLARE_POOLED_ALLOCATION(GC_Particle);
	DECLARE_SELF_REGISTRATION(GC_Particle);

public:
	float _time;
	float _time_life;
	bool  _fade;

	vec2d _velocity;

public:
	GC_Particle(const vec2d &pos, const vec2d &v, const TextureCache &texture, float LifeTime);
	GC_Particle(const vec2d &pos, const vec2d &v, const TextureCache &texture, float LifeTime, float orient);
	GC_Particle(FromFile);

	void SetFade(bool fade);

	virtual bool IsSaved() const { return true; }
	virtual void Serialize(SaveFile &f);

	virtual void TimeStepFloat(float dt);
};


// end of file
