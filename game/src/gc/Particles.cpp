#include "TypeReg.h"
#include "inc/gc/Particles.h"
#include "inc/gc/SaveFile.h"
#include "inc/gc/World.h"

IMPLEMENT_SELF_REGISTRATION(GC_BrickFragment)
{
	return true;
}

IMPLEMENT_1LIST_MEMBER(GC_BrickFragment, LIST_timestep);

GC_BrickFragment::GC_BrickFragment(vec2d pos, vec2d v0)
  : GC_Actor(pos)
  , _startFrame(rand())
  , _time(0)
  , _timeLife(frand(0.5f) + 1.0f)
  , _velocity(v0)
{
}

GC_BrickFragment::GC_BrickFragment(FromFile)
  : GC_Actor(FromFile())
{
}

void GC_BrickFragment::Serialize(World &world, SaveFile &f)
{
	GC_Actor::Serialize(world, f);
	f.Serialize(_startFrame);
	f.Serialize(_time);
	f.Serialize(_timeLife);
	f.Serialize(_velocity);
}

void GC_BrickFragment::TimeStep(World &world, float dt)
{
	_time += dt;

	if( _time >= _timeLife * 0.5f )
	{
		Kill(world);
		return;
	}

	MoveTo(world, GetPos() + _velocity * dt);
	_velocity += vec2d{ 0, 300.0f } *dt;
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Particle)
{
	return true;
}

IMPLEMENT_1LIST_MEMBER(GC_Particle, LIST_timestep);

GC_Particle::GC_Particle(vec2d pos, vec2d v, ParticleType ptype, float lifeTime, vec2d orient)
  : GC_Actor(pos)
  , _time(0)
  , _timeLife(lifeTime)
  , _rotationSpeed(0)
  , _velocity(v)
  , _ptype(ptype)
{
	assert(_timeLife > 0);
	SetDirection(orient);
}

GC_Particle::GC_Particle(FromFile)
  : GC_Actor(FromFile())
{
}

void GC_Particle::Serialize(World &world, SaveFile &f)
{
	GC_Actor::Serialize(world, f);
	f.Serialize(_sizeOverride);
	f.Serialize(_time);
	f.Serialize(_timeLife);
	f.Serialize(_rotationSpeed);
	f.Serialize(_velocity);
	f.Serialize(_ptype);
}

void GC_Particle::TimeStep(World &world, float dt)
{
	assert(_timeLife > 0);
	_time += dt;

	if( _time >= _timeLife )
	{
		Kill(world);
		return;
	}

	if( _rotationSpeed )
		SetDirection(Vec2dDirection(_rotationSpeed * _time));

	MoveTo(world, GetPos() + _velocity * dt);
}

void GC_Particle::SetFade(bool fade)
{
	SetFlags(GC_FLAG_PARTICLE_FADE, fade);
}

void GC_Particle::SetAutoRotate(float speed)
{
	_rotationSpeed = speed;
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_ParticleExplosion)
{
	return true;
}

IMPLEMENT_SELF_REGISTRATION(GC_ParticleDecal)
{
	return true;
}

IMPLEMENT_SELF_REGISTRATION(GC_ParticleGauss)
{
	return true;
}
