#include "TypeReg.h"
#include "inc/gc/Particles.h"
#include "inc/gc/SaveFile.h"
#include "inc/gc/World.h"

IMPLEMENT_SELF_REGISTRATION(GC_BrickFragment)
{
	return true;
}

IMPLEMENT_1LIST_MEMBER(GC_MovingObject, GC_BrickFragment, LIST_timestep);

GC_BrickFragment::GC_BrickFragment(vec2d pos, vec2d v0)
  : GC_MovingObject(pos)
  , _startFrame(rand())
  , _time(0)
  , _timeLife(frand(0.5f) + 1.0f)
  , _velocity(v0)
{
}

GC_BrickFragment::GC_BrickFragment(FromFile)
  : GC_MovingObject(FromFile())
{
}

void GC_BrickFragment::Serialize(World &world, SaveFile &f)
{
	GC_MovingObject::Serialize(world, f);
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

IMPLEMENT_1LIST_MEMBER(GC_MovingObject, GC_Particle, LIST_timestep);

GC_Particle::GC_Particle(vec2d pos, vec2d v, DecalType dtype, float lifeTime, float age)
  : GC_Decal(pos, dtype, lifeTime, age)
  , _velocity(v)
{
}

GC_Particle::GC_Particle(FromFile)
  : GC_Decal(FromFile())
{
}

void GC_Particle::Serialize(World &world, SaveFile &f)
{
	GC_MovingObject::Serialize(world, f);
	f.Serialize(_velocity);
}

void GC_Particle::TimeStep(World &world, float dt)
{
	MoveTo(world, GetPos() + _velocity * dt);
}
