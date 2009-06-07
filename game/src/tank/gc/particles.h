// particles.h

#pragma once

#include "2dSprite.h"

///////////////////////////////////////////////////////////////////////////////

class GC_Brick_Fragment_01 : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Brick_Fragment_01);

private:
	int _startFrame;

	float _time;
	float _timeLife;

	vec2d _velocity;

public:
	GC_Brick_Fragment_01(const vec2d &x0, const vec2d &v0);
	GC_Brick_Fragment_01(FromFile);

	virtual bool IsSaved() const { return true; }
	virtual void Serialize(SaveFile &f);

	virtual void TimeStepFloat(float dt);
};

///////////////////////////////////////////////////////////////////////////////

#define GC_FLAG_PARTICLE_FADE            (GC_FLAG_2DSPRITE_ << 0)
#define GC_FLAG_PARTICLE_                (GC_FLAG_2DSPRITE_ << 1)

class GC_Particle : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Particle);

public:
	float _time;
	float _timeLife;
	float _rotationSpeed;
	vec2d _velocity;

public:
	GC_Particle(const vec2d &pos, const vec2d &v, const TextureCache &texture, float lifeTime, float orient = 0);
	GC_Particle(FromFile);

	void SetFade(bool fade);
	void SetAutoRotate(float speed);

	virtual bool IsSaved() const { return true; }
	virtual void Serialize(SaveFile &f);

	virtual void TimeStepFloat(float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_ParticleScaled : public GC_Particle
{
	DECLARE_SELF_REGISTRATION(GC_ParticleScaled);

	float _size;

public:
	GC_ParticleScaled(const vec2d &pos, const vec2d &v, const TextureCache &texture, float lifeTime, float orient, float size);
	GC_ParticleScaled(FromFile);

	virtual void Serialize(SaveFile &f);
	virtual void Draw() const;
};

// end of file
