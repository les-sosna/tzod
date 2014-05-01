// particles.cpp

#include "particles.h"
#include "SaveFile.h"

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Brick_Fragment_01)
{
	return true;
}

GC_Brick_Fragment_01::GC_Brick_Fragment_01(Level &world, const vec2d &x0, const vec2d &v0)
  : GC_2dSprite(world)
  , _startFrame(rand())
  , _time(0)
  , _timeLife(frand(0.5f) + 1.0f)
  , _velocity(v0)
{
	static TextureCache tex("particle_brick");

	SetTexture(tex);
	SetZ(world, Z_PARTICLE);
	MoveTo(world, x0);
	SetEvents(world, GC_FLAG_OBJECT_EVENTS_TS_FIXED);
}

GC_Brick_Fragment_01::GC_Brick_Fragment_01(FromFile)
  : GC_2dSprite(FromFile())
{
}

void GC_Brick_Fragment_01::Serialize(Level &world, SaveFile &f)
{
	GC_2dSprite::Serialize(world, f);
	f.Serialize(_startFrame);
	f.Serialize(_time);
	f.Serialize(_timeLife);
	f.Serialize(_velocity);
}

void GC_Brick_Fragment_01::TimeStepFloat(Level &world, float dt)
{
	_time += dt;

	if( _time >= _timeLife * 0.5f )
	{
		Kill(world);
		return;
	}

	SetFrame(int((float)_startFrame + (float)(GetFrameCount() - 1) *
		_time / _timeLife)%(GetFrameCount() - 1) );

	MoveTo(world, GetPos() + _velocity * dt);
	_velocity += vec2d(0, 300.0f) * dt;
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Particle)
{
	return true;
}

GC_Particle::GC_Particle(Level &world, const vec2d &pos, const vec2d &v, const TextureCache &texture,
                         float lifeTime, const vec2d &orient)
  : GC_2dSprite(world)
  , _time(0)
  , _timeLife(lifeTime)
  , _rotationSpeed(0)
  , _rotationPhase(0)
  , _velocity(v)
{
	assert(_timeLife > 0);

	SetZ(world, Z_PARTICLE);

	SetTexture(texture);
	SetDirection(orient);

	MoveTo(world, pos);
	SetEvents(world, GC_FLAG_OBJECT_EVENTS_TS_FIXED);
}

GC_Particle::GC_Particle(FromFile)
  : GC_2dSprite(FromFile())
{
}

void GC_Particle::Serialize(Level &world, SaveFile &f)
{
	GC_2dSprite::Serialize(world, f);
	f.Serialize(_time);
	f.Serialize(_timeLife);
	f.Serialize(_rotationSpeed);
	f.Serialize(_rotationPhase);
	f.Serialize(_velocity);
}

void GC_Particle::TimeStepFloat(Level &world, float dt)
{
	assert(_timeLife > 0);
	_time += dt;

	if( _time >= _timeLife )
	{
		Kill(world);
		return;
	}

	SetFrame( int((float)(GetFrameCount() - 1) * _time / _timeLife) );

	if( CheckFlags(GC_FLAG_PARTICLE_FADE) )
		SetOpacity(1.0f - _time / _timeLife);

	if( _rotationSpeed )
		SetDirection(vec2d(_rotationPhase + _rotationSpeed * _time));

	MoveTo(world, GetPos() + _velocity * dt);
}

void GC_Particle::SetFade(bool fade)
{
	SetFlags(GC_FLAG_PARTICLE_FADE, fade);
	if( fade )
		SetOpacity(1.0f - _time / _timeLife);
	else
		SetOpacity1i(255);
}

void GC_Particle::SetAutoRotate(float speed)
{
	_rotationSpeed = speed;
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_ParticleScaled)
{
	return true;
}

GC_ParticleScaled::GC_ParticleScaled(Level &world, const vec2d &pos, const vec2d &v, const TextureCache &texture,
                                     float lifeTime, const vec2d &orient, float size)
  : GC_Particle(world, pos, v, texture, lifeTime, orient)
  , _size(size)
{
}

GC_ParticleScaled::GC_ParticleScaled(FromFile)
  : GC_Particle(FromFile())
{
}

void GC_ParticleScaled::Serialize(Level &world, SaveFile &f)
{
	GC_Particle::Serialize(world, f);
	f.Serialize(_size);
}

void GC_ParticleScaled::Draw(bool editorMode) const
{
	g_texman->DrawSprite(GetTexture(), GetCurrentFrame(), GetColor(), 
		GetPos().x, GetPos().y, _size, _size, GetDirection());
}

///////////////////////////////////////////////////////////////////////////////
// end of file
