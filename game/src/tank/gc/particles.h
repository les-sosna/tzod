// particles.h

#pragma once

#include "2dSprite.h"

///////////////////////////////////////////////////////////////////////////////

class GC_BrickFragment : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_BrickFragment);
    typedef GC_2dSprite base;

private:
	int _startFrame;

	float _time;
	float _timeLife;

	vec2d _velocity;

public:
    DECLARE_LIST_MEMBER();
	GC_BrickFragment(World &world, const vec2d &v0);
	GC_BrickFragment(FromFile);

	// GC_2dSprite
	virtual enumZOrder GetZ() const { return Z_PARTICLE; }
	
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStepFloat(World &world, float dt);
};

///////////////////////////////////////////////////////////////////////////////

enum ParticleType
{
	PARTICLE_FIRE1,
	PARTICLE_FIRE2,
	PARTICLE_FIRE3,
	PARTICLE_FIRE4,
	PARTICLE_FIRESPARK,
	PARTICLE_TYPE1,
	PARTICLE_TYPE2,
	PARTICLE_TYPE3,
	PARTICLE_TRACE1,
	PARTICLE_TRACE2,
	PARTICLE_SMOKE,
	PARTICLE_EXPLOSION1,
	PARTICLE_EXPLOSION2,
	PARTICLE_EXPLOSION_G,
	PARTICLE_EXPLOSION_E,
	PARTICLE_EXPLOSION_S,
	PARTICLE_EXPLOSION_P,
	PARTICLE_BIGBLAST,
	PARTICLE_SMALLBLAST,
	PARTICLE_GAUSS1,
	PARTICLE_GAUSS2,
	PARTICLE_GAUSS_HIT,
	PARTICLE_GREEN,
	PARTICLE_YELLOW,
	PARTICLE_CATTRACK,
};

#define GC_FLAG_PARTICLE_FADE            (GC_FLAG_2DSPRITE_ << 0)
#define GC_FLAG_PARTICLE_                (GC_FLAG_2DSPRITE_ << 1)

class GC_Particle : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Particle);
    typedef GC_2dSprite base;

public:
	float _sizeOverride = -1;
	float _time;
	float _timeLife;
	float _rotationSpeed;
	vec2d _velocity;
	ParticleType _ptype = PARTICLE_TYPE1;

public:
    DECLARE_LIST_MEMBER();
	GC_Particle(World &world, const vec2d &v, ParticleType ptype, float lifeTime, const vec2d &orient = vec2d(1,0));
	GC_Particle(FromFile);
	
	ParticleType GetParticleType() const { return _ptype; }
	bool GetFade() const { return CheckFlags(GC_FLAG_PARTICLE_FADE); }
	float GetTime() const { return _time; }
	float GetLifeTime() const { return _timeLife; }
	float GetRotationSpeed() const { return _rotationSpeed; }
	float GetSizeOverride() const { return _sizeOverride; }

	void SetFade(bool fade);
	void SetAutoRotate(float speed);
	void SetSizeOverride(float size) { _sizeOverride = size; }

	// GC_2dSprite
	virtual enumZOrder GetZ() const { return Z_PARTICLE; }
	
	// GC_Object
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStepFloat(World &world, float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_ParticleExplosion : public GC_Particle
{
	DECLARE_SELF_REGISTRATION(GC_ParticleExplosion);
public:
	using GC_Particle::GC_Particle;
	// GC_2dSprite
	virtual enumZOrder GetZ() const { return Z_EXPLODE; }
};

class GC_ParticleDecal : public GC_Particle
{
	DECLARE_SELF_REGISTRATION(GC_ParticleDecal);
public:
	using GC_Particle::GC_Particle;
	// GC_2dSprite
	virtual enumZOrder GetZ() const { return Z_WATER; }
};

class GC_ParticleGauss : public GC_Particle
{
	DECLARE_SELF_REGISTRATION(GC_ParticleGauss);
public:
	using GC_Particle::GC_Particle;
	// GC_2dSprite
	virtual enumZOrder GetZ() const { return Z_GAUSS_RAY; }
};

// end of file
