#include "TypeReg.h"
#include <gc/Projectiles.h>
#include <gc/Explosion.h>
#include <gc/GameClasses.h>
#include <gc/Light.h>
#include <gc/Macros.h>
#include <gc/Particles.h>
#include <gc/Player.h>
#include <gc/RigidBodyDinamic.h>
#include <gc/WeapCfg.h>
#include <gc/World.h>
#include <gc/WorldEvents.h>
#include <gc/SaveFile.h>

IMPLEMENT_1LIST_MEMBER(GC_Projectile, LIST_timestep);

GC_Projectile::GC_Projectile(vec2d pos, vec2d v, GC_RigidBodyStatic *ignore, GC_Player *owner, bool advanced, bool trail)
  : GC_Actor(pos)
  , _velocity(v.len())
  , _ignore(ignore)
  , _owner(owner)
  , _trailDensity(10.0f)
  , _trailPath(0.0f)
{
	SetFlags(GC_FLAG_PROJECTILE_ADVANCED, advanced);
	SetFlags(GC_FLAG_PROJECTILE_TRAIL, trail);

	vec2d dir(v);
	dir.Normalize();
	SetDirection(dir);
}

GC_Projectile::GC_Projectile(FromFile)
  : GC_Actor(FromFile())
{
}

GC_Projectile::~GC_Projectile()
{
}

void GC_Projectile::Init(World &world)
{
	GC_Actor::Init(world);
	_light = &world.New<GC_Light>(GetPos(), GC_Light::LIGHT_POINT);
	_trailPath = world.net_frand(_trailDensity);
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

	f.Serialize(_trailDensity);
	f.Serialize(_trailPath);
	f.Serialize(_velocity);
	f.Serialize(_light);
	f.Serialize(_owner);
	f.Serialize(_ignore);
}

void GC_Projectile::MoveTo(World &world, const vec2d &pos)
{
	_light->MoveTo(world, pos);
	GC_Actor::MoveTo(world, pos);
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

	MoveTo(world, pos);
}

static void ApplyHitDamage(World &world, GC_RigidBodyStatic &target, const DamageDesc &dd, vec2d impulse)
{
	if( GC_RigidBodyDynamic *dyn = dynamic_cast<GC_RigidBodyDynamic *>(&target) )
	{
		dyn->ApplyImpulse(impulse, dd.hit);
	}
	if( dd.damage >= 0 )
	{
		target.TakeDamage(world, dd);
	}
	else
	{
		target.SetHealth(std::min(target.GetHealth() - dd.damage, target.GetHealthMax()));
	}
}

void GC_Projectile::TimeStep(World &world, float dt)
{
	GC_Actor::TimeStep(world, dt);

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
		for( auto it = obstacles.begin(); obstacles.end() != it; ++it )
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
            assert(!std::isnan(relativeDepth) && std::isfinite(relativeDepth));
			assert(relativeDepth >= 0);

			for( auto ls: world.eGC_Projectile._listeners )
				ls->OnHit(*this, *it->obj, enter);

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

void GC_Projectile::SetTrailDensity(float density)
{
	_trailDensity = density;
	_trailPath = frand(density);
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Rocket)
{
	return true;
}

GC_Rocket::GC_Rocket(vec2d pos, vec2d v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(pos, v, ignore, owner, advanced, true)
  , _timeHomming(0.0f)
{
	SetTrailDensity(1.5f);
}

GC_Rocket::GC_Rocket(FromFile)
  : GC_Projectile(FromFile())
{
}

GC_Rocket::~GC_Rocket()
{
}

void GC_Rocket::SelectTarget(World &world)
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
		if( !world.CalcOutstrip(GetPos(), GetVelocity(), veh->GetPos(), veh->_lv, target) )
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

void GC_Rocket::Serialize(World &world, SaveFile &f)
{
	GC_Projectile::Serialize(world, f);
	f.Serialize(_timeHomming);
	f.Serialize(_target);
}

bool GC_Rocket::OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth)
{
	auto &e = world.New<GC_ExplosionStandard>(hit + norm);
	e.SetDamage(DAMAGE_ROCKET_AK47);
	e.SetOwner(GetOwner());
	
	DamageDesc dd;
	dd.damage = 0; //world.net_frand(10.0f);
	dd.hit = hit;
	dd.from = GetOwner();
	ApplyHitDamage(world, *object, dd, GetDirection() * 15);
	
	Kill(world);
	return true;
}

void GC_Rocket::SpawnTrailParticle(World &world, const vec2d &pos)
{
	world.New<GC_Particle>(pos - GetDirection() * 8.0f,
						   GetDirection() * (GetVelocity() * 0.3f),
						   _target ? PARTICLE_FIRE2:PARTICLE_FIRE1,
						   frand(0.1f) + 0.02f);
}

void GC_Rocket::TimeStep(World &world, float dt)
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
			world.CalcOutstrip(GetPos(), GetVelocity(), _target->GetPos(), _target->_lv, target);

			vec2d a = target - GetPos();

			vec2d p = a - GetDirection();
			vec2d dv = p - GetDirection() * (GetDirection() * p);

			float ldv = dv.len();
			if( ldv > 0 )
			{
				dv /= (ldv * GetVelocity());
				dv *= WEAP_RL_HOMING_FACTOR;

				vec2d dir(GetDirection());
				dir += dv * dt;
				dir.Normalize();
				SetDirection(dir);
			}
		}
	}

	GC_Projectile::TimeStep(world, dt);
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Bullet)
{
	return true;
}

GC_Bullet::GC_Bullet(vec2d pos, vec2d v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(pos, v, ignore, owner, advanced, true)
  , _trailEnable(false)
{
	SetTrailDensity(5.0f);
}

GC_Bullet::GC_Bullet(FromFile)
  : GC_Projectile(FromFile())
{
}

void GC_Bullet::Init(World &world)
{
	GC_Projectile::Init(world);
	_light->SetActive(false);	
}

void GC_Bullet::Serialize(World &world, SaveFile &f)
{
	GC_Projectile::Serialize(world, f);
	f.Serialize(_trailEnable);
}

bool GC_Bullet::OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth)
{
	float n = norm.Angle();
	float a1 = n - 1.4f;
	float a2 = n + 1.4f;
	for( int i = 0; i < 7; ++i )
	{
		vec2d a(a1 + frand(a2 - a1));
		world.New<GC_Particle>(hit, a * (frand(50.0f) + 50.0f), PARTICLE_TRACE1, frand(0.1f) + 0.03f, a);
	}

	auto &light = world.New<GC_Light>(hit, GC_Light::LIGHT_POINT);
	light.SetRadius(50);
	light.SetIntensity(0.5f);
	light.SetTimeout(world, 0.3f);

	DamageDesc dd;
	dd.damage = GetAdvanced() ? DAMAGE_BULLET * 2 : DAMAGE_BULLET;
	dd.hit = hit;
	dd.from = GetOwner();
	ApplyHitDamage(world, *object, dd, GetDirection() * 5);

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
		world.New<GC_Particle>(pos, vec2d(0,0), PARTICLE_TRACE2, frand(0.01f) + 0.09f, GetDirection());
	}
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_TankBullet)
{
	return true;
}

GC_TankBullet::GC_TankBullet(vec2d pos, vec2d v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(pos, v, ignore, owner, advanced, true)
{
	SetTrailDensity(5.0f);
}

GC_TankBullet::GC_TankBullet(FromFile)
  : GC_Projectile(FromFile())
{
}

void GC_TankBullet::Init(World &world)
{
	GC_Projectile::Init(world);
	_light->SetActive(GetAdvanced());
}

bool GC_TankBullet::OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth)
{
	if( GetAdvanced() )
	{
		auto &e = world.New<GC_ExplosionBig>(vec2d(std::max(.0f, std::min(world._sx - 1, hit.x + norm.x)),
												   std::max(.0f, std::min(world._sy - 1, hit.y + norm.y))));
		e.SetOwner(GetOwner());
        e.SetTimeout(world, 0.05f);
	}
	else
	{
		float a = norm.Angle();
		float a1 = a - 1.4f;
		float a2 = a + 1.4f;
		for( int n = 0; n < 9; n++ )
		{
			vec2d v(a1 + frand(a2 - a1));
			world.New<GC_Particle>(hit, v * (frand(100.0f) + 50.0f), PARTICLE_TRACE1, frand(0.2f) + 0.05f, v);
		}

		auto &light = world.New<GC_Light>(hit, GC_Light::LIGHT_POINT);
		light.SetRadius(80);
		light.SetIntensity(1.5f);
		light.SetTimeout(world, 0.3f);

		world.New<GC_Particle>(hit, vec2d(0,0), PARTICLE_EXPLOSION_S, 0.3f, vrand(1));
	}

	DamageDesc dd;
	dd.damage = DAMAGE_TANKBULLET;
	dd.hit = hit;
	dd.from = GetOwner();
	ApplyHitDamage(world, *object, dd, GetDirection() * 100);
	
	Kill(world);
	return true;
}

void GC_TankBullet::SpawnTrailParticle(World &world, const vec2d &pos)
{
	world.New<GC_Particle>(pos, vec2d(0,0), GetAdvanced() ? PARTICLE_TRACE1:PARTICLE_TRACE2, frand(0.05f) + 0.05f, GetDirection());
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_PlazmaClod)
{
	return true;
}

GC_PlazmaClod::GC_PlazmaClod(vec2d pos, vec2d v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(pos, v, ignore, owner, advanced, true)
{
	SetTrailDensity(4.0f);
}

GC_PlazmaClod::GC_PlazmaClod(FromFile)
  : GC_Projectile(FromFile())
{
}

bool GC_PlazmaClod::OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth)
{
	if( GetAdvanced() )
	{
		auto &daemon = world.New<GC_HealthDaemon>(object->GetPos(), GetOwner(), 15.0f, 2.0f);
        daemon.SetVictim(world, object);
	}

	float a = norm.Angle();
	float a1 = a - 1.5f;
	float a2 = a + 1.5f;
	for( int n = 0; n < 15; n++ )
	{
		vec2d v(a1 + frand(a2 - a1));
		world.New<GC_Particle>(hit, v * (frand(100.0f) + 50.0f), PARTICLE_GREEN, frand(0.2f) + 0.05f, v);
	}

	auto &light = world.New<GC_Light>(hit, GC_Light::LIGHT_POINT);
	light.SetRadius(90);
	light.SetIntensity(1.5f);
	light.SetTimeout(world, 0.4f);

	world.New<GC_Particle>(hit, vec2d(0,0), PARTICLE_EXPLOSION_P, 0.3f, vrand(1));

	DamageDesc dd;
	dd.damage = DAMAGE_PLAZMA;
	dd.hit = hit;
	dd.from = GetOwner();
	ApplyHitDamage(world, *object, dd, vec2d(0, 0));
	
	Kill(world);
	return true;
}

void GC_PlazmaClod::SpawnTrailParticle(World &world, const vec2d &pos)
{
	world.New<GC_Particle>(pos, vec2d(0,0), PARTICLE_GREEN, frand(0.15f) + 0.10f);
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_BfgCore)
{
	return true;
}

GC_BfgCore::GC_BfgCore(vec2d pos, vec2d v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(pos, v, ignore, owner, advanced, true)
{
	SetTrailDensity(2.5f);
}

GC_BfgCore::GC_BfgCore(FromFile)
  : GC_Projectile(FromFile())
{
}

GC_BfgCore::~GC_BfgCore()
{
}

void GC_BfgCore::Init(World &world)
{
	GC_Projectile::Init(world);
	_light->SetRadius(WEAP_BFG_RADIUS * 2);
	_target = FindTarget(world);
}

GC_RigidBodyDynamic* GC_BfgCore::FindTarget(World &world) const
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
	return nearest_cosinus > 0.87f ? pNearestTarget : nullptr;
}

void GC_BfgCore::Serialize(World &world, SaveFile &f)
{
	GC_Projectile::Serialize(world, f);
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
		world.New<GC_Particle>(hit, vec2d(a1 + frand(a2 - a1)) * (frand(100.0f) + 50.0f), PARTICLE_GREEN, frand(0.3f) + 0.15f);
	}


	auto &light = world.New<GC_Light>(hit, GC_Light::LIGHT_POINT);
	light.SetRadius(WEAP_BFG_RADIUS * 3);
	light.SetIntensity(1.5f);
	light.SetTimeout(world, 0.5f);

	world.New<GC_Particle>(hit, vec2d(0,0), PARTICLE_EXPLOSION_G, 0.3f);

	DamageDesc dd;
	dd.damage = DAMAGE_BFGCORE;
	dd.hit = hit;
	dd.from = GetOwner();
	ApplyHitDamage(world, *object, dd, vec2d(0, 0));
	
	Kill(world);
	return true;
}

void GC_BfgCore::SpawnTrailParticle(World &world, const vec2d &pos)
{
	vec2d dx = vrand(WEAP_BFG_RADIUS) * frand(1.0f);
	world.New<GC_Particle>(pos + dx, vrand(7.0f), PARTICLE_GREEN, 0.7f);
}

void GC_BfgCore::TimeStep(World &world, float dt)
{
	if (auto target = FindTarget(world))
		_target = target;

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
			veh->TakeDamage(world, DamageDesc{damage * DAMAGE_BFGCORE * dt, veh->GetPos() + d, GetOwner()});
		}
	});

	//------------------------------------------

	if( _target )
	{
		vec2d target;
		world.CalcOutstrip(GetPos(), GetVelocity(), _target->GetPos(), _target->_lv, target);

		vec2d a = target - GetPos();

		vec2d p = a - GetDirection();
		vec2d dv = p - GetDirection() * (GetDirection() * p);

		float ldv = dv.len();
		if( ldv > 0 )
		{
			dv /= (ldv * GetVelocity());
			dv *= (3.0f * fabs(_target->_lv.len()) / WEAP_BFG_TARGET_SPEED + (GetAdvanced() ? 1 : 0)) * WEAP_BFG_HOMMING_FACTOR;

			vec2d dir(GetDirection());
			dir += dv * dt;
			dir.Normalize();
			SetDirection(dir);
		}
	}

	GC_Projectile::TimeStep(world, dt);
}


/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_FireSpark)
{
	return true;
}

GC_FireSpark::GC_FireSpark(vec2d pos, vec2d v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(pos, v, ignore, owner, advanced, true)
  , _time(0)
  , _timeLife(1)
  , _rotation(frand(10) - 5)
{
	SetTrailDensity(4.5f);
}

GC_FireSpark::GC_FireSpark(FromFile)
  : GC_Projectile(FromFile())
{
}

void GC_FireSpark::Init(World &world)
{
	GC_Projectile::Init(world);
	_light->SetRadius(0);
	_light->SetIntensity(0.5f);
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
	SetVelocity(GetVelocity() * 0.9f / (1 + up * 4));

	if( GetAdvanced() && GetOwner() != object->GetOwner()
		&& (world.net_rand()&1) && CheckFlags(GC_FLAG_FIRESPARK_SETFIRE) )
	{
		auto &daemon = world.New<GC_HealthDaemon>(object->GetPos(), GetOwner(), 10.0f, 3.0f);
        daemon.SetVictim(world, object);
	}

	DamageDesc dd;
	dd.damage = DAMAGE_FIRE_HIT;
	dd.hit = hit;
	dd.from = GetOwner();
	
	if( GetAdvanced() && GetOwner() != object->GetOwner() )
		dd.damage = CheckFlags(GC_FLAG_FIRESPARK_HEALOWNER) ? -dd.damage : 0;

	ApplyHitDamage(world, *object, dd, vec2d(0, 0));

	if( GC_RigidBodyStatic *tmp = world.TraceNearest(world.grid_rigid_s, object, hit, norm) )
	{
		if( tmp->GetOwner() != GetOwner() )
		{
			ApplyHitDamage(world, *tmp, dd, vec2d(0, 0));
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
	auto &p = world.New<GC_Particle>(pos + vrand(3),
									 GetDirection() * (GetVelocity()/3) + vrand(10.0f),
									 PARTICLE_FIRESPARK,
									 0.1f + frand(0.3f),
									 vrand(1));
	p.SetFade(true);
	p.SetAutoRotate(_rotation);
	p.SetSizeOverride(GetRadius());

	// random walk
	vec2d tmp = GetDirection() + vec2d(GetDirection().y, -GetDirection().x) * (world.net_frand(0.06f) - 0.03f);
	SetDirection(tmp.Normalize());
}

void GC_FireSpark::TimeStep(World &world, float dt)
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
					object->TakeDamage(world, DamageDesc{damage, object->GetPos() + d, GetOwner()});
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
		GC_Projectile::TimeStep(world, dt);

		// correct particle's position as if it was affected by air friction
		if( watch )
		{
			const float a = 1.5;
			float e = exp(-a * dt);
			vec2d correcton = GetDirection() * (GetVelocity() * ((1 - e) / a - dt));
			SetVelocity(GetVelocity() * e);
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

GC_ACBullet::GC_ACBullet(vec2d pos, vec2d v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(pos, v, ignore, owner, advanced, true)
{
	SetTrailDensity(5.0f);
}

GC_ACBullet::GC_ACBullet(FromFile)
  : GC_Projectile(FromFile())
{
}

void GC_ACBullet::Init(World &world)
{
	GC_Projectile::Init(world);
	_light->SetRadius(30);
	_light->SetIntensity(0.6f);
	_light->SetActive(GetAdvanced());
}

bool GC_ACBullet::OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth)
{
	float a = norm.Angle();
	float a1 = a - 1.0f;
	float a2 = a + 1.0f;
	for(int i = 0; i < 12; i++)
	{
		vec2d dir(a1 + frand(a2 - a1));
		world.New<GC_Particle>(hit, dir * frand(300.0f), PARTICLE_TRACE1, frand(0.05f) + 0.05f, dir);
	}

	auto &light = world.New<GC_Light>(hit + norm * 5.0f, GC_Light::LIGHT_POINT);
	light.SetRadius(80);
	light.SetIntensity(1.5f);
	light.SetTimeout(world, 0.1f);

	DamageDesc dd;
	dd.damage = DAMAGE_ACBULLET;
	dd.hit = hit;
	dd.from = GetOwner();
	ApplyHitDamage(world, *object, dd, GetDirection() * 20);
	
	Kill(world);
	return true;
}

void GC_ACBullet::SpawnTrailParticle(World &world, const vec2d &pos)
{
	world.New<GC_Particle>(pos, vec2d(0,0), PARTICLE_TRACE2, frand(0.05f) + 0.05f, GetDirection());
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_GaussRay)
{
	return true;
}

GC_GaussRay::GC_GaussRay(vec2d pos, vec2d v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(pos, v, ignore, owner, advanced, true)
  , _damage(DAMAGE_GAUSS)
{
	SetTrailDensity(16.0f);
}

GC_GaussRay::GC_GaussRay(FromFile)
  : GC_Projectile(FromFile())
{
}

void GC_GaussRay::Init(World &world)
{
	GC_Projectile::Init(world);
	
	SAFE_KILL(world, _light);
	_light = &world.New<GC_Light>(GetPos(), GC_Light::LIGHT_DIRECT);
	_light->SetRadius(64);
	_light->SetLength(0);
	_light->SetIntensity(1.5f);
	_light->SetLightDirection(-GetDirection());
}

void GC_GaussRay::Kill(World &world)
{
	if( _light ) // _light can be killed during level cleanup
		_light->SetTimeout(world, 0.4f);
    GC_Projectile::Kill(world);
}

void GC_GaussRay::Serialize(World &world, SaveFile &f)
{
	GC_Projectile::Serialize(world, f);
	f.Serialize(_damage);
}

void GC_GaussRay::SpawnTrailParticle(World &world, const vec2d &pos)
{
	auto &p = world.New<GC_ParticleGauss>(pos, vec2d(0,0), GetAdvanced() ? PARTICLE_GAUSS2 : PARTICLE_GAUSS1, 0.2f, GetDirection());
	p.SetFade(true);

	_light->SetLength(_light->GetLength() + GetTrailDensity());
}

bool GC_GaussRay::OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth)
{
	auto &p = world.New<GC_Particle>(hit, vec2d(0,0), PARTICLE_GAUSS_HIT, 0.5f, vec2d(norm.y, -norm.x));
    p.SetFade(true);
	
	DamageDesc dd;
	dd.damage = _damage * relativeDepth;
	dd.hit = hit;
	dd.from = GetOwner();
	ApplyHitDamage(world, *object, dd, GetDirection() * _damage / DAMAGE_GAUSS * 100);

	if( GetAdvanced() )
		relativeDepth /= 4;
	_damage -= relativeDepth * DAMAGE_GAUSS_FADE;
	if( _damage <= 0 )
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

GC_Disk::GC_Disk(vec2d pos, vec2d v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(pos, v, ignore, owner, advanced, true)
  , _bounces(0)
{
	SetTrailDensity(5.0f);
}

GC_Disk::GC_Disk(FromFile)
  : GC_Projectile(FromFile())
{
}

void GC_Disk::Init(World &world)
{
	GC_Projectile::Init(world);
	_light->SetActive(false);
}

void GC_Disk::Serialize(World &world, SaveFile &f)
{
	GC_Projectile::Serialize(world, f);
	f.Serialize(_bounces);
}

bool GC_Disk::OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth)
{
	DamageDesc dd;
	dd.damage = (float) (_bounces + 1) * DAMAGE_DISK_FADE;
	dd.hit = hit;
	dd.from = GetOwner();
	if( GetAdvanced() && GetOwner() == object->GetOwner() )
		dd.damage /= 3; // one third of damage to owner
	ApplyHitDamage(world, *object, dd, GetDirection() * dd.damage / DAMAGE_DISK_MAX * 20);

	SetDirection(GetDirection() - norm * 2 * (GetDirection() * norm));
	MoveWithTrail(world, hit + norm, true);

	for( int i = 0; i < 11; ++i )
	{
		vec2d v = (norm + vrand(frand(1.0f))) * 100.0f;
		vec2d vnorm = v;
		vnorm.Normalize();
		world.New<GC_Particle>(hit, v, PARTICLE_TRACE1, frand(0.2f) + 0.02f, vnorm);
	}

	if( _bounces == 0 )
	{
		float a = norm.Angle();

		float a1 = a - PI / 3;
		float a2 = a + PI / 3;

		for( int n = 0; n < 14; ++n )
		{
			world.New<GC_Bullet>(GetPos(),
								 vec2d(a1 + world.net_frand(a2 - a1)) * (world.net_frand(2000.0f) + 3000.0f),
								 GetIgnore(),
								 GetOwner(),
								 GetAdvanced());
		}

		world.New<GC_Particle>(hit, vec2d(0,0), PARTICLE_EXPLOSION_E, 0.2f, vrand(1));

		auto &light = world.New<GC_Light>(hit, GC_Light::LIGHT_POINT);
		light.SetRadius(100);
		light.SetIntensity(1.5f);
		light.SetTimeout(world, 0.2f);

		Kill(world);
		return true;
	}
	
	_bounces--;

	if( GetAdvanced() )
	{
		float a = norm.Angle();

		float a1 = a - PI / 4;
		float a2 = a + PI / 4;

		for( int n = 0; n < 11; ++n )
		{
			world.New<GC_Bullet>(GetPos(),
								 vec2d(a1 + world.net_frand(a2 - a1)) * (world.net_frand(2000.0f) + 3000.0f),
								 GetIgnore(),
								 GetOwner(),
								 true);
		}
	}

	auto &light = world.New<GC_Light>(hit + norm * 5.0f, GC_Light::LIGHT_POINT);
	light.SetRadius(70);
	light.SetIntensity(1.5f);
	light.SetTimeout(world, 0.1f);

	return true;
}

void GC_Disk::SpawnTrailParticle(World &world, const vec2d &pos)
{
	vec2d dx = vrand(3.0f);
	float time = frand(0.01f) + 0.03f;

	vec2d v = (-dx - GetDirection() * (-dx * GetDirection())) / time;
	vec2d dir(v - GetDirection() * (32.0f / time));
	dir.Normalize();
	world.New<GC_Particle>(pos + dx - GetDirection()*4.0f, v, PARTICLE_TRACE2, time, dir);
}
