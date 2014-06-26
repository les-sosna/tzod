// particles.h

#pragma once

#include "2dSprite.h"

///////////////////////////////////////////////////////////////////////////////

class GC_Brick_Fragment_01 : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Brick_Fragment_01);
    typedef GC_2dSprite base;

private:
	int _startFrame;

	float _time;
	float _timeLife;

	vec2d _velocity;

public:
    DECLARE_LIST_MEMBER();
	GC_Brick_Fragment_01(World &world, const vec2d &v0);
	GC_Brick_Fragment_01(FromFile);

	// GC_2dSprite
	virtual enumZOrder GetZ() const { return Z_PARTICLE; }
	
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStepFloat(World &world, float dt);
};

///////////////////////////////////////////////////////////////////////////////

#define GC_FLAG_PARTICLE_FADE            (GC_FLAG_2DSPRITE_ << 0)
#define GC_FLAG_PARTICLE_                (GC_FLAG_2DSPRITE_ << 1)

class GC_Particle : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Particle);
    typedef GC_2dSprite base;

public:
	float _time;
	float _timeLife;
	float _rotationSpeed;
	vec2d _velocity;
	enumZOrder _zOrder;

public:
    DECLARE_LIST_MEMBER();
	GC_Particle(World &world, enumZOrder zOrder, const vec2d &v, const TextureCache &texture, float lifeTime, const vec2d &orient = vec2d(1,0));
	GC_Particle(FromFile);

	void SetFade(bool fade);
	void SetAutoRotate(float speed);

	// GC_2dSprite
	virtual enumZOrder GetZ() const { return _zOrder; }
	
	// GC_Object
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStepFloat(World &world, float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_ParticleScaled : public GC_Particle
{
	DECLARE_SELF_REGISTRATION(GC_ParticleScaled);

	float _size;

public:
	GC_ParticleScaled(World &world, const vec2d &v, const TextureCache &texture, float lifeTime, const vec2d &orient, float size);
	GC_ParticleScaled(FromFile);

	virtual void Serialize(World &world, SaveFile &f);
	virtual void Draw(DrawingContext &dc, bool editorMode) const;
};

// end of file
