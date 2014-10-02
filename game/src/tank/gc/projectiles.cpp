// projectiles.cpp
///////////////////////////////////////////////////////////////////////////////

#include "projectiles.h"

#include "Explosion.h"
#include "GameClasses.h"
#include "World.h"
#include "Light.h"
#include "Macros.h"
#include "particles.h"
#include "Player.h"
#include "RigidBodyDinamic.h"
#include "Sound.h"
#include "SaveFile.h"

#include "config/Config.h"

IMPLEMENT_1LIST_MEMBER(GC_Projectile, LIST_timestep);

GC_Projectile::GC_Projectile(World &world, GC_RigidBodyStatic *ignore, GC_Player *owner,
							 bool advanced, bool trail, const vec2d &pos, const vec2d &v)
  : _ignore(ignore)
  , _owner(owner)
  , _light(new GC_Light(world, GC_Light::LIGHT_POINT))
  , _velocity(v.len())
  , _hitDamage(0)
  , _hitImpulse(0)
  , _trailDensity(10.0f)
  , _trailPath(0.0f)
{
    _light->Register(world);
    
	SetFlags(GC_FLAG_PROJECTILE_ADVANCED, advanced);
	SetFlags(GC_FLAG_PROJECTILE_TRAIL, trail);

	MoveWithTrail(world, pos, false);

	vec2d dir(v);
	dir.Normalize();
	SetDirection(dir);
}

GC_Projectile::GC_Projectile(FromFile)
{
}

GC_Projectile::~GC_Projectile()
{
}

void GC_Projectile::Kill(World &world)
{
	if( _light && _light->GetTimeout() <= 0 )
	{
		_light->Kill(world);
	}
    GC_Actor::Kill(world);
}

void GC_Projectile::Serialize(World &world, SaveFile &f)
{
	GC_Actor::Serialize(world, f);

	f.Serialize(_hitDamage);
	f.Serialize(_hitImpulse);
	f.Serialize(_trailDensity);
	f.Serialize(_trailPath);
	f.Serialize(_velocity);
	f.Serialize(_light);
	f.Serialize(_owner);
	f.Serialize(_ignore);
}

void GC_Projectile::MoveWithTrail(World &world, const vec2d &pos, bool trail)
{
	if( trail )
	{
		vec2d e = pos - GetPos();
		float len = e.len();

		e /= len;

		while( _trailPath < len )
		{
			SpawnTrailParticle(world, GetPos() + e * _trailPath);
			_trailPath += _trailDensity;
		}

		_trailPath -= len;
	}
	else
	{
		_trailPath = world.net_frand(_trailDensity);
	}

	_light->MoveTo(world, pos);
	GC_Actor::MoveTo(world, pos);
}

float GC_Projectile::FilterDamage(float damage, GC_RigidBodyStatic *object)
{
	return damage;
}

void GC_Projectile::ApplyHitDamage(World &world, GC_RigidBodyStatic *target, const vec2d &hitPoint)
{
	if( GC_RigidBodyDynamic *dyn = dynamic_cast<GC_RigidBodyDynamic *>(target) )
	{
		dyn->ApplyImpulse(GetDirection() * _hitImpulse, hitPoint);
	}
	float damage = FilterDamage(_hitDamage, target);
	if( damage >= 0 )
	{
		target->TakeDamage(world, damage, hitPoint, _owner);
	}
	else
	{
		// heal
		target->SetHealth(std::min(target->GetHealth() - damage, target->GetHealthMax()));
	}
}

void GC_Projectile::TimeStepFixed(World &world, float dt)
{
	GC_Actor::TimeStepFixed(world, dt);

	vec2d dx = GetDirection() * (_velocity * dt);
	std::vector<World::CollisionPoint> obstacles;
	world.TraceAll(world.grid_rigid_s, GetPos(), dx, obstacles);

	if( !obstacles.empty() )
	{
		struct cmp 
		{
			bool operator () (const World::CollisionPoint &left, const World::CollisionPoint &right)
			{
				return left.enter < right.enter;
			}
		};
		std::sort(obstacles.begin(), obstacles.end(), cmp());
		for( std::vector<World::CollisionPoint>::const_iterator it = obstacles.begin(); obstacles.end() != it; ++it )
		{
			if( _ignore == it->obj )
			{
				continue;
			}
			_ignore = NULL;

			vec2d enter = GetPos() + dx * (it->enter + 0.5f);
			float depth = it->exit - it->enter;
			float relativeDepth = depth > std::numeric_limits<float>::epsilon() ?
				(std::min(.5f, it->exit) - std::max(-.5f, it->enter)) / depth : 1;
			assert(!isnan(relativeDepth) && isfinite(relativeDepth));
			assert(relativeDepth >= 0);

			ObjPtr<GC_Projectile> watch(this);
			bool stop = OnHit(world, it->obj, enter, it->normal, relativeDepth);
			if( stop )
			{
				if( watch )
					_ignore = NULL;
				return;
			}
		}
	}

	MoveWithTrail(world, GetPos() + dx, CheckFlags(GC_FLAG_PROJECTILE_TRAIL));
	if( GetPos().x < 0 || GetPos().x > world._sx ||
		GetPos().y < 0 || GetPos().y > world._sy )
	{
		Kill(world);
	}
}

void GC_Projectile::SetTrailDensity(World &world, float density)
{
	_trailDensity = density;
	_trailPath = world.net_frand(density);
}

void GC_Projectile::SetHitDamage(float damage)
{
	_hitDamage = damage;
}

void GC_Projectile::SetHitImpulse(float impulse)
{
	_hitImpulse = impulse;
}

// inline
GC_RigidBodyStatic* GC_Projectile::GetIgnore() const
{
	return _ignore;
}


///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Rocket)
{
	return true;
}

GC_Rocket::GC_Rocket(World &world, const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(world, ignore, owner, advanced, true, x, v)
  , _timeHomming(0.0f)
{
	SetTrailDensity(world, 1.5f);
	SetHitImpulse(15);

	auto sound = new GC_Sound_link(world, SND_RocketFly, this);
    sound->Register(world);
    sound->SetMode(world, SMODE_LOOP);

	// advanced rocket searches for target
	if( GetAdvanced() )
	{
		GC_RigidBodyDynamic *pNearestTarget = NULL; // by angle
		float nearest_cosinus = 0;

		FOREACH( world.GetList(LIST_vehicles), GC_RigidBodyDynamic, veh )
		{
			if( GetOwner() == veh->GetOwner() )
				continue;

			if( veh != world.TraceNearest(world.grid_rigid_s, GetIgnore(), GetPos(), veh->GetPos() - GetPos()) )
			{
				// target invisible
				continue;
			}

			vec2d target;
			if( !world.CalcOutstrip(GetPos(), _velocity, veh->GetPos(), veh->_lv, target) )
			{
				// target is moving too fast
				continue;
			}

			vec2d a = target - GetPos();

			float cosinus = (a * GetDirection()) / a.len();

			if( cosinus > nearest_cosinus )
			{
				nearest_cosinus = cosinus;
				pNearestTarget = veh;
			}
		}

		// select only if less than 20 degrees
		if( nearest_cosinus > 0.94f )
		{
			_target = pNearestTarget;
		}
	}

	PLAY(SND_RocketShoot, GetPos());
}

GC_Rocket::GC_Rocket(FromFile)
  : GC_Projectile(FromFile())
{
}

GC_Rocket::~GC_Rocket()
{
}

void GC_Rocket::Serialize(World &world, SaveFile &f)
{
	GC_Projectile::Serialize(world, f);
	f.Serialize(_timeHomming);
	f.Serialize(_target);
}

bool GC_Rocket::OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth)
{
	auto &e = MakeExplosionStandard(world, hit + norm, GetOwner());
    e.SetDamage(DAMAGE_ROCKET_AK47);
	ApplyHitDamage(world, object, hit);
	Kill(world);
	return true;
}

void GC_Rocket::SpawnTrailParticle(World &world, const vec2d &pos)
{
	auto p = new GC_Particle(world, GetDirection() * (_velocity * 0.3f), _target ? PARTICLE_FIRE2:PARTICLE_FIRE1, frand(0.1f) + 0.02f);
    p->Register(world);
    p->MoveTo(world, pos - GetDirection() * 8.0f);
}

void GC_Rocket::TimeStepFixed(World &world, float dt)
{
	_timeHomming += dt;

	if( _target )
	{
		if( WEAP_RL_HOMING_TIME < _timeHomming )
		{
			_target = NULL;
		}
		else
		{
			vec2d target;
			world.CalcOutstrip(GetPos(), _velocity, _target->GetPos(), _target->_lv, target);

			vec2d a = target - GetPos();

			vec2d p = a - GetDirection();
			vec2d dv = p - GetDirection() * (GetDirection() * p);

			float ldv = dv.len();
			if( ldv > 0 )
			{
				dv /= (ldv * _velocity);
				dv *= WEAP_RL_HOMING_FACTOR;

				vec2d dir(GetDirection());
				dir += dv * dt;
				dir.Normalize();
				SetDirection(dir);
			}
		}
	}

	GC_Projectile::TimeStepFixed(world, dt);
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Bullet)
{
	return true;
}

GC_Bullet::GC_Bullet(World &world, const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(world, ignore, owner, advanced, true, x, v)
  , _trailEnable(false)
{
	SetHitDamage(advanced ? DAMAGE_BULLET * 2 : DAMAGE_BULLET);
	SetHitImpulse(5);
	SetTrailDensity(world, 5.0f);
	_light->SetActive(false);
}

GC_Bullet::GC_Bullet(FromFile)
  : GC_Projectile(FromFile())
{
}

GC_Bullet::~GC_Bullet()
{
}

void GC_Bullet::Serialize(World &world, SaveFile &f)
{
	GC_Projectile::Serialize(world, f);
	f.Serialize(_trailEnable);
}

bool GC_Bullet::OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth)
{
	//
	// spawn particles
	//
	float n = norm.Angle();
	float a1 = n - 1.4f;
	float a2 = n + 1.4f;
	for( int i = 0; i < 7; ++i )
	{
		vec2d a(a1 + frand(a2 - a1));
		auto p = new GC_Particle(world, a * (frand(50.0f) + 50.0f), PARTICLE_TRACE1, frand(0.1f) + 0.03f, a);
        p->Register(world);
        p->MoveTo(world, hit);
	}

	GC_Light *pLight = new GC_Light(world, GC_Light::LIGHT_POINT);
    pLight->Register(world);
	pLight->MoveTo(world, hit);
	pLight->SetRadius(50);
	pLight->SetIntensity(0.5f);
	pLight->SetTimeout(world, 0.3f);

	ApplyHitDamage(world, object, hit);
	Kill(world);
	return true;
}

void GC_Bullet::SpawnTrailParticle(World &world, const vec2d &pos)
{
	if( !(rand() & (_trailEnable ? 0x1f : 0x7F)) )
	{
		_trailEnable = !_trailEnable;
		_light->SetActive(_trailEnable);
	}

	if( _trailEnable )
	{
		auto p = new GC_Particle(world, vec2d(0,0), PARTICLE_TRACE2, frand(0.01f) + 0.09f, GetDirection());
        p->Register(world);
        p->MoveTo(world, pos);
	}
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_TankBullet)
{
	return true;
}

GC_TankBullet::GC_TankBullet(World &world, const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(world, ignore, owner, advanced, true, x, v)
{
	SetTrailDensity(world, 5.0f);
	SetHitDamage(DAMAGE_TANKBULLET);
	SetHitImpulse(100);
	_light->SetActive(advanced);
	PLAY(SND_Shoot, GetPos());
}

GC_TankBullet::GC_TankBullet(FromFile)
  : GC_Projectile(FromFile())
{
}

GC_TankBullet::~GC_TankBullet()
{
}

bool GC_TankBullet::OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth)
{
	if( GetAdvanced() )
	{
		auto &e = MakeExplosionBig(world, vec2d(std::max(.0f, std::min(world._sx - 1, hit.x + norm.x)),
		                                        std::max(.0f, std::min(world._sy - 1, hit.y + norm.y))), GetOwner());
        e.SetBoomTimeout(0.05f);
	}
	else
	{
		float a = norm.Angle();
		float a1 = a - 1.4f;
		float a2 = a + 1.4f;
		for( int n = 0; n < 9; n++ )
		{
			vec2d v(a1 + frand(a2 - a1));
			auto p = new GC_Particle(world, v * (frand(100.0f) + 50.0f), PARTICLE_TRACE1, frand(0.2f) + 0.05f, v);
            p->Register(world);
            p->MoveTo(world, hit);
		}

		GC_Light *pLight = new GC_Light(world, GC_Light::LIGHT_POINT);
        pLight->Register(world);
		pLight->MoveTo(world, hit);
		pLight->SetRadius(80);
		pLight->SetIntensity(1.5f);
		pLight->SetTimeout(world, 0.3f);

		auto p = new GC_Particle(world, vec2d(0,0), PARTICLE_EXPLOSION_S, 0.3f, vrand(1));
        p->Register(world);
        p->MoveTo(world, hit);
		PLAY(SND_BoomBullet, hit);
	}

	ApplyHitDamage(world, object, hit);
	Kill(world);
	return true;
}

void GC_TankBullet::SpawnTrailParticle(World &world, const vec2d &pos)
{
	auto p = new GC_Particle(world, vec2d(0,0), GetAdvanced() ? PARTICLE_TRACE1:PARTICLE_TRACE2, frand(0.05f) + 0.05f, GetDirection());
    p->Register(world);
    p->MoveTo(world, pos);
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_PlazmaClod)
{
	return true;
}

GC_PlazmaClod::GC_PlazmaClod(World &world, const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(world, ignore, owner, advanced, true, x, v)
{
	SetHitDamage(DAMAGE_PLAZMA);
	SetTrailDensity(world, 4.0f);

	PLAY(SND_PlazmaFire, GetPos());
}

GC_PlazmaClod::GC_PlazmaClod(FromFile)
  : GC_Projectile(FromFile())
{
}

GC_PlazmaClod::~GC_PlazmaClod()
{
}

bool GC_PlazmaClod::OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth)
{
	if( GetAdvanced() )
	{
		auto daemon = new GC_HealthDaemon(world, GetOwner(), 15.0f, 2.0f);
        daemon->Register(world);
        daemon->SetVictim(world, object);
	}

	float a = norm.Angle();
	float a1 = a - 1.5f;
	float a2 = a + 1.5f;
	for( int n = 0; n < 15; n++ )
	{
		vec2d v(a1 + frand(a2 - a1));
		auto p = new GC_Particle(world, v * (frand(100.0f) + 50.0f), PARTICLE_GREEN, frand(0.2f) + 0.05f, v);
        p->Register(world);
        p->MoveTo(world, hit);
	}

	GC_Light *pLight = new GC_Light(world, GC_Light::LIGHT_POINT);
    pLight->Register(world);
	pLight->MoveTo(world, hit);
	pLight->SetRadius(90);
	pLight->SetIntensity(1.5f);
	pLight->SetTimeout(world, 0.4f);

	auto p = new GC_Particle(world, vec2d(0,0), PARTICLE_EXPLOSION_P, 0.3f, vrand(1));
    p->Register(world);
    p->MoveTo(world, hit);
	PLAY(SND_PlazmaHit, hit);

	ApplyHitDamage(world, object, hit);
	Kill(world);
	return true;
}

void GC_PlazmaClod::SpawnTrailParticle(World &world, const vec2d &pos)
{
	auto p = new GC_Particle(world, vec2d(0,0), PARTICLE_GREEN, frand(0.15f) + 0.10f);
    p->Register(world);
    p->MoveTo(world, pos);
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_BfgCore)
{
	return true;
}

GC_BfgCore::GC_BfgCore(World &world, const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(world, ignore, owner, advanced, true, x, v)
  , _time(0)
{
	PLAY(SND_BfgFire, GetPos());

	SetTrailDensity(world, 2.5f);
	SetHitDamage(DAMAGE_BFGCORE);

	FindTarget(world);

	_light->SetRadius(WEAP_BFG_RADIUS * 2);
}

GC_BfgCore::GC_BfgCore(FromFile)
  : GC_Projectile(FromFile())
{
}

GC_BfgCore::~GC_BfgCore()
{
}

void GC_BfgCore::FindTarget(World &world)
{
	GC_RigidBodyDynamic *pNearestTarget = NULL; // by angle
	float nearest_cosinus = 0;

	FOREACH( world.GetList(LIST_vehicles), GC_RigidBodyDynamic, veh )
	{
		if( GetOwner() == veh->GetOwner() ) continue;

		// check target visibility
		if( veh != world.TraceNearest(world.grid_rigid_s,
			GetIgnore(), GetPos(), veh->GetPos() - GetPos()) ) continue;

		vec2d a = veh->GetPos() - GetPos();

		float cosinus = (a * GetDirection()) / a.len();

		if( cosinus > nearest_cosinus )
		{
			nearest_cosinus = cosinus;
			pNearestTarget = veh;
		}
	}

	// accept only if the angle is less than 30 degrees
	if( nearest_cosinus > 0.87f )
		_target = pNearestTarget;
}

void GC_BfgCore::Serialize(World &world, SaveFile &f)
{
	GC_Projectile::Serialize(world, f);
	f.Serialize(_time);
	f.Serialize(_target);
}

bool GC_BfgCore::OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth)
{
	float a = norm.Angle();
	float a1 = a - 1.4f;
	float a2 = a + 1.4f;
	for(int n = 0; n < 64; n++)
	{
		//ring
		auto p = new GC_Particle(world, vec2d(a1 + frand(a2 - a1)) * (frand(100.0f) + 50.0f), PARTICLE_GREEN, frand(0.3f) + 0.15f);
        p->Register(world);
        p->MoveTo(world, hit);
	}


	GC_Light *pLight = new GC_Light(world, GC_Light::LIGHT_POINT);
    pLight->Register(world);
	pLight->MoveTo(world, hit);
	pLight->SetRadius(WEAP_BFG_RADIUS * 3);
	pLight->SetIntensity(1.5f);
	pLight->SetTimeout(world, 0.5f);

	auto p = new GC_Particle(world, vec2d(0,0), PARTICLE_EXPLOSION_G, 0.3f);
    p->Register(world);
    p->MoveTo(world, hit);
	PLAY(SND_BfgFlash, hit);

	ApplyHitDamage(world, object, hit);
	Kill(world);
	return true;
}

void GC_BfgCore::SpawnTrailParticle(World &world, const vec2d &pos)
{
	vec2d dx = vrand(WEAP_BFG_RADIUS) * frand(1.0f);
	auto p = new GC_Particle(world, vrand(7.0f), PARTICLE_GREEN, 0.7f);
    p->Register(world);
    p->MoveTo(world, pos + dx);
}

void GC_BfgCore::TimeStepFixed(World &world, float dt)
{
	_time += dt;
	if( _time * ANIMATION_FPS >= 1 )
	{
		_time -= 1/ANIMATION_FPS;
		FindTarget(world);
	}

	world.GetList(LIST_vehicles).for_each([&](ObjectList::id_type, GC_Object *o)
	{
        auto veh = static_cast<GC_RigidBodyDynamic*>(o);
		const float R = WEAP_BFG_RADIUS;
		float damage = (1 - (GetPos() - veh->GetPos()).len() / R) *
			(fabs(veh->_lv.len()) / SPEED_BFGCORE * 10 + 0.5f);

		if( damage > 0 && !(GetAdvanced() && GetOwner() == veh->GetOwner()) )
		{
			vec2d delta(GetPos() - veh->GetPos());
			delta.Normalize();
			vec2d d = delta + world.net_vrand(1.0f);
			veh->TakeDamage(world, damage * DAMAGE_BFGCORE * dt, veh->GetPos() + d, GetOwner());
		}
	});

	//------------------------------------------

	if( _target )
	{
		vec2d target;
		world.CalcOutstrip(GetPos(), _velocity, _target->GetPos(), _target->_lv, target);

		vec2d a = target - GetPos();

		vec2d p = a - GetDirection();
		vec2d dv = p - GetDirection() * (GetDirection() * p);

		float ldv = dv.len();
		if( ldv > 0 )
		{
			dv /= (ldv * _velocity);
			dv *= (3.0f * fabs(_target->_lv.len()) / WEAP_BFG_TARGET_SPEED + (GetAdvanced() ? 1 : 0)) * WEAP_BFG_HOMMING_FACTOR;

			vec2d dir(GetDirection());
			dir += dv * dt;
			dir.Normalize();
			SetDirection(dir);
		}
	}

	GC_Projectile::TimeStepFixed(world, dt);
}


/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_FireSpark)
{
	return true;
}

GC_FireSpark::GC_FireSpark(World &world, const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(world, ignore, owner, advanced, true, x, v)
  , _time(0)
  , _timeLife(1)
  , _rotation(frand(10) - 5)
{
	SetHitDamage(DAMAGE_FIRE_HIT);
	SetTrailDensity(world, 4.5f);
	_light->SetRadius(0);
	_light->SetIntensity(0.5f);
}

GC_FireSpark::GC_FireSpark(FromFile)
  : GC_Projectile(FromFile())
{
}

GC_FireSpark::~GC_FireSpark()
{
}

void GC_FireSpark::Serialize(World &world, SaveFile &f)
{
	GC_Projectile::Serialize(world, f);
	f.Serialize(_time);
	f.Serialize(_timeLife);
	f.Serialize(_rotation);
}

bool GC_FireSpark::OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth)
{
	vec2d nn(norm.y, -norm.x);

	if( GetDirection() * nn < (world.net_frand(0.6f) - 0.3f) )
	{
		nn = -nn;
		_rotation = frand(4) + 5;
	}
	else
	{
		_rotation = -frand(4) - 5;
	}


	float vdotn = GetDirection() * norm;
	float up = (0.5f - 0.6f * world.net_frand(vdotn));
	up *= up;
	up *= up;
	up *= up;
	up *= up;

	nn += norm * up;
	nn.Normalize();

	SetDirection(nn);
	_velocity *= 0.9f / (1 + up * 4);

	if( GetAdvanced() && GetOwner() != object->GetOwner()
		&& (world.net_rand()&1) && CheckFlags(GC_FLAG_FIRESPARK_SETFIRE) )
	{
		auto daemon = new GC_HealthDaemon(world, GetOwner(), 10.0f, 3.0f);
        daemon->Register(world);
        daemon->SetVictim(world, object);
	}

	ApplyHitDamage(world, object, hit);

	if( GC_RigidBodyStatic *tmp = world.TraceNearest(world.grid_rigid_s, object, hit, norm) )
	{
		if( tmp->GetOwner() != GetOwner() )
		{
			ApplyHitDamage(world, tmp, hit);
			Kill(world);
		}
	}
	else
	{
		MoveWithTrail(world, hit + norm, true);
	}
	return true;
}

void GC_FireSpark::SpawnTrailParticle(World &world, const vec2d &pos)
{
	GC_Particle *p = new GC_Particle(world, GetDirection() * (_velocity/3) + vrand(10.0f), PARTICLE_FIRESPARK, 0.1f + frand(0.3f), vrand(1));
    p->Register(world);
    p->MoveTo(world, pos + vrand(3));
	p->SetFade(true);
	p->SetAutoRotate(_rotation);
	p->SetSizeOverride(GetRadius());

	// random walk
	vec2d tmp = GetDirection() + vec2d(GetDirection().y, -GetDirection().x) * (world.net_frand(0.06f) - 0.03f);
	SetDirection(tmp.Normalize());
}

float GC_FireSpark::FilterDamage(float damage, GC_RigidBodyStatic *object)
{
	if( GetAdvanced() && GetOwner() != object->GetOwner() )
	{
		return CheckFlags(GC_FLAG_FIRESPARK_HEALOWNER) ? -damage : 0;
	}
	return damage;
}

void GC_FireSpark::TimeStepFixed(World &world, float dt)
{
	float R = GetRadius();
	_light->SetRadius(3*R);

	R *= 1.5; // for damage calculation

    std::vector<ObjectList*> receive;
	world.grid_rigid_s.OverlapPoint(receive, GetPos() / LOCATION_SIZE);

	const bool healOwner = CheckFlags(GC_FLAG_FIRESPARK_HEALOWNER);

	for( auto it1 = receive.begin(); it1 != receive.end(); ++it1 )
	{
		(*it1)->for_each([&](ObjectList::id_type, GC_Object *o)
		{
            auto object = static_cast<GC_RigidBodyStatic*>(o);
			vec2d dist = GetPos() - object->GetPos();
			float destLen = dist.len();

			float damage = (1 - destLen / R) * DAMAGE_FIRE * dt;
			if( damage > 0 )
			{
				if( GetAdvanced() && GetOwner() == object->GetOwner() )
				{
					if( healOwner )
					{
						object->SetHealth(std::min(object->GetHealth() + damage, object->GetHealthMax()));
					}
				}
				else
				{
					vec2d d = dist.Normalize() + world.net_vrand(1.0f);
					object->TakeDamage(world, damage, object->GetPos() + d, GetOwner());
				}
			}
		});
	}

	_time += dt;
	if( _time > _timeLife )
	{
		Kill(world);
	}
	else
	{
		ObjPtr<GC_FireSpark> watch;

		// this moves the particle by velocity*dt
		GC_Projectile::TimeStepFixed(world, dt);

		// correct particle's position as if it was affected by air friction
		if( watch )
		{
			const float a = 1.5;
			float e = exp(-a * dt);
			vec2d correcton = GetDirection() * (_velocity * ((1 - e) / a - dt));
			_velocity *= e;
			MoveWithTrail(world, GetPos() + correcton, false);
		}
	}
}

void GC_FireSpark::SetLifeTime(float t)
{
	_timeLife = t;
}

void GC_FireSpark::SetHealOwner(bool heal)
{
	SetFlags(GC_FLAG_FIRESPARK_HEALOWNER, heal);
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_ACBullet)
{
	return true;
}

GC_ACBullet::GC_ACBullet(World &world, const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(world, ignore, owner, advanced, true, x, v)
{
	SetHitDamage(DAMAGE_ACBULLET);
	SetHitImpulse(20);
	SetTrailDensity(world, 5.0f);
	_light->SetRadius(30);
	_light->SetIntensity(0.6f);
	_light->SetActive(advanced);
}

GC_ACBullet::GC_ACBullet(FromFile)
  : GC_Projectile(FromFile())
{
}

GC_ACBullet::~GC_ACBullet()
{
}

bool GC_ACBullet::OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth)
{
	if( dynamic_cast<GC_Wall_Concrete *>(object) )
	{
		switch (rand() % 2)
		{
		case 0:
			PLAY(SND_AC_Hit2, hit);
			break;
		case 1:
			PLAY(SND_AC_Hit3, hit);
			break;
		}
	}
	else
	{
		PLAY(SND_AC_Hit1, hit);
	}


	float a = norm.Angle();
	float a1 = a - 1.0f;
	float a2 = a + 1.0f;
	for(int i = 0; i < 12; i++)
	{
		vec2d dir(a1 + frand(a2 - a1));
		auto p = new GC_Particle(world, dir * frand(300.0f), PARTICLE_TRACE1, frand(0.05f) + 0.05f, dir);
        p->Register(world);
        p->MoveTo(world, hit);
	}

	GC_Light *pLight = new GC_Light(world, GC_Light::LIGHT_POINT);
    pLight->Register(world);
	pLight->MoveTo(world, hit + norm * 5.0f);
	pLight->SetRadius(80);
	pLight->SetIntensity(1.5f);
	pLight->SetTimeout(world, 0.1f);

	ApplyHitDamage(world, object, hit);
	Kill(world);
	return true;
}

void GC_ACBullet::SpawnTrailParticle(World &world, const vec2d &pos)
{
	auto p = new GC_Particle(world, vec2d(0,0), PARTICLE_TRACE2, frand(0.05f) + 0.05f, GetDirection());
    p->Register(world);
    p->MoveTo(world, pos);
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_GaussRay)
{
	return true;
}

GC_GaussRay::GC_GaussRay(World &world, const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(world, ignore, owner, advanced, true, x, v)
{
	SetHitDamage(DAMAGE_GAUSS);
	SetHitImpulse(100);
	SetTrailDensity(world, 16.0f);

	PLAY(SND_Bolt, GetPos());

	SAFE_KILL(world, _light);

	_light = new GC_Light(world, GC_Light::LIGHT_DIRECT);
    _light->Register(world);
	_light->SetRadius(64);
	_light->SetLength(0);
	_light->SetIntensity(1.5f);
	vec2d d = -v;
	d.Normalize();
	_light->SetLightDirection(d);
	_light->MoveTo(world, GetPos());
}

GC_GaussRay::GC_GaussRay(FromFile)
  : GC_Projectile(FromFile())
{
}

GC_GaussRay::~GC_GaussRay()
{
}

void GC_GaussRay::Kill(World &world)
{
	if( _light ) // _light can be killed during level cleanup
		_light->SetTimeout(world, 0.4f);
    GC_Projectile::Kill(world);
}

void GC_GaussRay::SpawnTrailParticle(World &world, const vec2d &pos)
{
	GC_Particle *p = new GC_ParticleGauss(world, vec2d(0,0), GetAdvanced() ? PARTICLE_GAUSS2 : PARTICLE_GAUSS1, 0.2f, GetDirection());
    p->Register(world);
    p->MoveTo(world, pos);
	p->SetFade(true);

	_light->SetLength(_light->GetLength() + GetTrailDensity());
}

bool GC_GaussRay::OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth)
{
	auto p = new GC_Particle(world, vec2d(0,0), PARTICLE_GAUSS_HIT, 0.5f, vec2d(norm.y, -norm.x));
    p->Register(world);
    p->MoveTo(world, hit);
    p->SetFade(true);

	//ApplyHitDamage(object, hit);
	if( GC_RigidBodyDynamic *dyn = dynamic_cast<GC_RigidBodyDynamic *>(object) )
	{
		dyn->ApplyImpulse(GetDirection() * GetHitDamage() / DAMAGE_GAUSS * 100, hit);
	}
	float damage = FilterDamage(GetHitDamage(), object);
	if( damage >= 0 )
	{
		object->TakeDamage(world, damage * relativeDepth, hit, GetOwner());
	}


	if( GetAdvanced() )
		relativeDepth /= 4;
	SetHitDamage(GetHitDamage() - relativeDepth * DAMAGE_GAUSS_FADE);
	SetHitImpulse(GetHitDamage() / DAMAGE_GAUSS * 100);
	if( GetHitDamage() <= 0 )
	{
		MoveWithTrail(world, hit, CheckFlags(GC_FLAG_PROJECTILE_TRAIL)); // workaround to see trail at last step
		Kill(world);
		return true;
	}
	return false; // don't stop
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Disk)
{
	return true;
}

GC_Disk::GC_Disk(World &world, const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(world, ignore, owner, advanced, true, x, v)
{
	SetHitDamage(world.net_frand(DAMAGE_DISK_MAX - DAMAGE_DISK_MIN) + DAMAGE_DISK_MIN * (advanced ? 2.0f : 1.0f));
	SetHitImpulse(GetHitDamage() / DAMAGE_DISK_MAX * 20);
	SetTrailDensity(world, 5.0f);
	_light->SetActive(false);
}

GC_Disk::GC_Disk(FromFile)
  : GC_Projectile(FromFile())
{
}

GC_Disk::~GC_Disk()
{
}

bool GC_Disk::OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth)
{
	ApplyHitDamage(world, object, hit);

	SetDirection(GetDirection() - norm * 2 * (GetDirection() * norm));
	MoveWithTrail(world, hit + norm, true);

	for( int i = 0; i < 11; ++i )
	{
		vec2d v = (norm + vrand(frand(1.0f))) * 100.0f;
		vec2d vnorm = v;
		vnorm.Normalize();
		auto p = new GC_Particle(world, v, PARTICLE_TRACE1, frand(0.2f) + 0.02f, vnorm);
        p->Register(world);
        p->MoveTo(world, hit);
	}

	SetHitDamage(GetHitDamage() - DAMAGE_DISK_FADE);
	SetHitImpulse(GetHitDamage() / DAMAGE_DISK_MAX * 20);

	if( GetHitDamage() <= 0 )
	{
		float a = norm.Angle();

		float a1 = a - PI / 3;
		float a2 = a + PI / 3;

		for( int n = 0; n < 14; ++n )
		{
			(new GC_Bullet(world,
				GetPos(),
				vec2d(a1 + world.net_frand(a2 - a1)) * (world.net_frand(2000.0f) + 3000.0f),
				GetIgnore(),
				GetOwner(),
				GetAdvanced()))->Register(world);
		}

		auto p = new GC_Particle(world, vec2d(0,0), PARTICLE_EXPLOSION_E, 0.2f, vrand(1));
        p->Register(world);
        p->MoveTo(world, hit);

		GC_Light *pLight = new GC_Light(world, GC_Light::LIGHT_POINT);
        pLight->Register(world);
		pLight->MoveTo(world, hit);
		pLight->SetRadius(100);
		pLight->SetIntensity(1.5f);
		pLight->SetTimeout(world, 0.2f);

		PLAY(SND_BoomBullet, hit);
		Kill(world);
		return true;
	}

	PLAY(SND_DiskHit, hit);
	if( GetAdvanced() )
	{
		float a = norm.Angle();

		float a1 = a - PI / 4;
		float a2 = a + PI / 4;

		for( int n = 0; n < 11; ++n )
		{
			(new GC_Bullet(world,
				GetPos(),
				vec2d(a1 + world.net_frand(a2 - a1)) * (world.net_frand(2000.0f) + 3000.0f),
				GetIgnore(),
				GetOwner(),
				true))->Register(world);
		}
	}

	GC_Light *pLight = new GC_Light(world, GC_Light::LIGHT_POINT);
    pLight->Register(world);
	pLight->MoveTo(world, hit + norm * 5.0f);
	pLight->SetRadius(70);
	pLight->SetIntensity(1.5f);
	pLight->SetTimeout(world, 0.1f);

	return true;
}

void GC_Disk::SpawnTrailParticle(World &world, const vec2d &pos)
{
	vec2d dx = vrand(3.0f);
	float time = frand(0.01f) + 0.03f;

	vec2d v = (-dx - GetDirection() * (-dx * GetDirection())) / time;
	vec2d dir(v - GetDirection() * (32.0f / time));
	dir.Normalize();
	auto p = new GC_Particle(world, v, PARTICLE_TRACE2, time, dir);
    p->Register(world);
    p->MoveTo(world, pos + dx - GetDirection()*4.0f);
}

float GC_Disk::FilterDamage(float damage, GC_RigidBodyStatic *object)
{
	if( GetAdvanced() && GetOwner() == object->GetOwner() )
	{
		return damage / 3; // one third of damage to owner
	}
	return damage;
}

///////////////////////////////////////////////////////////////////////////////
// end of file
