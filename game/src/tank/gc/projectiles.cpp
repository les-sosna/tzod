// projectiles.cpp
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "projectiles.h"

#include "level.h"
#include "macros.h"
#include "functions.h"

#include "fs/SaveFile.h"

#include "config/Config.h"

#include "GameClasses.h"
#include "light.h"
#include "sound.h"
#include "particles.h"
#include "Player.h"
#include "RigidBodyDinamic.h"

///////////////////////////////////////////////////////////////////////////////

GC_Projectile::GC_Projectile(GC_RigidBodyStatic *ignore, GC_Player *owner, bool advanced, bool trail,
                             const vec2d &pos, const vec2d &v, const char *texture)
  : GC_2dSprite()
  , _memberOf(this)
  , _light(new GC_Light(GC_Light::LIGHT_POINT))
  , _owner(owner)
  , _ignore(ignore)
  , _hitDamage(0)
  , _trailDensity(10.0f)
  , _trailPath(0.0f)
  , _velocity(v.len())
  , _hitImpulse(0)
{
	SetZ(Z_PROJECTILE);
	SetShadow(true);

	SetFlags(GC_FLAG_PROJECTILE_ADVANCED, advanced);
	SetFlags(GC_FLAG_PROJECTILE_TRAIL, trail);

	SetTexture(texture);
	MoveTo(pos, false);

	vec2d dir(v);
	dir.Normalize();
	SetDirection(dir);

	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FIXED);
}

GC_Projectile::GC_Projectile(FromFile)
  : GC_2dSprite(FromFile())
  , _memberOf(this)
{
}

GC_Projectile::~GC_Projectile()
{
}

void GC_Projectile::Kill()
{
	_owner  = NULL;
	_ignore = NULL;
	if( _light )
	{
		if( _light->GetTimeout() > 0 )
			_light = NULL;
		else
			SAFE_KILL(_light);
	}
	GC_2dSprite::Kill();
}

void GC_Projectile::Serialize(SaveFile &f)
{
	GC_2dSprite::Serialize(f);

	f.Serialize(_hitDamage);
	f.Serialize(_hitImpulse);
	f.Serialize(_trailDensity);
	f.Serialize(_trailPath);
	f.Serialize(_velocity);
	f.Serialize(_light);
	f.Serialize(_owner);
	f.Serialize(_ignore);
}

void GC_Projectile::MoveTo(const vec2d &pos, bool trail)
{
	if( trail )
	{
		vec2d e = pos - GetPos();
		float len = e.len();

		e /= len;

		while( _trailPath < len )
		{
			SpawnTrailParticle(GetPos() + e * _trailPath);
			_trailPath += _trailDensity;
		}

		_trailPath -= len;
	}
	else
	{
		_trailPath = g_level->net_frand(_trailDensity);
	}

	_light->MoveTo(pos);
	GC_2dSprite::MoveTo(pos);
}

float GC_Projectile::FilterDamage(float damage, GC_RigidBodyStatic *object)
{
	return damage;
}

void GC_Projectile::ApplyHitDamage(GC_RigidBodyStatic *target, const vec2d &hitPoint)
{
	assert(!target->IsKilled());
	if( GC_RigidBodyDynamic *dyn = dynamic_cast<GC_RigidBodyDynamic *>(target) )
	{
		dyn->ApplyImpulse(GetDirection() * _hitImpulse, hitPoint);
	}
	float damage = FilterDamage(_hitDamage, target);
	if( damage >= 0 )
	{
		target->TakeDamage(damage, hitPoint, GetRawPtr(_owner));
	}
	else
	{
		// heal
		target->SetHealthCur(std::min(target->GetHealth() - damage, target->GetHealthMax()));
	}
}

void GC_Projectile::TimeStepFixed(float dt)
{
	GC_2dSprite::TimeStepFixed(dt);

	vec2d dx = GetDirection() * (_velocity * dt);
	std::vector<Level::CollisionPoint> obstacles;
	g_level->TraceAll(g_level->grid_rigid_s, GetPos(), dx, obstacles);

	if( !obstacles.empty() )
	{
		struct cmp 
		{
			bool operator () (const Level::CollisionPoint &left, const Level::CollisionPoint &right)
			{
				return left.enter < right.enter;
			}
		};
		std::sort(obstacles.begin(), obstacles.end(), cmp());
		for( std::vector<Level::CollisionPoint>::const_iterator it = obstacles.begin(); obstacles.end() != it; ++it )
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
			assert(!_isnan(relativeDepth) && _finite(relativeDepth));
			assert(relativeDepth >= 0);

			AddRef();
			bool stop = OnHit(it->obj, enter, it->normal, relativeDepth);
			if( stop )
			{
				if( !IsKilled() )
					_ignore = NULL;
				Release();
				return;
			}
			Release();
		}
	}

	MoveTo(GetPos() + dx, CheckFlags(GC_FLAG_PROJECTILE_TRAIL));
	if( GetPos().x < 0 || GetPos().x > g_level->_sx ||
		GetPos().y < 0 || GetPos().y > g_level->_sy )
	{
		Kill();
	}
}

void GC_Projectile::SetTrailDensity(float density)
{
	_trailDensity = density;
	_trailPath = g_level->net_frand(density);
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
	return _ignore && !_ignore->IsKilled() ? GetRawPtr(_ignore) : NULL;
}


///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Rocket)
{
	return true;
}

GC_Rocket::GC_Rocket(const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(ignore, owner, advanced, TRUE, x, v, "projectile_rocket")
  , _timeHomming(0.0f)
{
	SetTrailDensity(1.5f);
	SetHitImpulse(15);

	new GC_Sound_link(SND_RocketFly, SMODE_LOOP, this);

	// advanced rocket searches for target
	if( GetAdvanced() )
	{
		GC_RigidBodyDynamic *pNearestTarget = NULL; // by angle
		float nearest_cosinus = 0;

		FOREACH( g_level->GetList(LIST_vehicles), GC_RigidBodyDynamic, veh )
		{
			if( veh->IsKilled() || GetOwner() == veh->GetOwner() )
				continue;

			if( veh != g_level->TraceNearest(g_level->grid_rigid_s, GetIgnore(), GetPos(), veh->GetPos() - GetPos()) )
			{
				// target invisible
				continue;
			}

			vec2d target;
			if( !g_level->CalcOutstrip(GetPos(), _velocity, veh->GetPos(), veh->_lv, target) )
			{
				// target is moving too fast
				continue;
			}

			vec2d a = target - GetPos();

			// косинус угла направления на цель
			float cosinus = (a * GetDirection()) / a.len();

			if( cosinus > nearest_cosinus )
			{
				nearest_cosinus = cosinus;
				pNearestTarget = veh;
			}
		}

		// выбираем только если ближе 20 градусов
		if( nearest_cosinus > 0.94f )
		{
			_target = WrapRawPtr(pNearestTarget);
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

void GC_Rocket::Kill()
{
	FreeTarget();
	GC_Projectile::Kill();
}

void GC_Rocket::Serialize(SaveFile &f)
{
	GC_Projectile::Serialize(f);
	f.Serialize(_timeHomming);
	f.Serialize(_target);
}

bool GC_Rocket::OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth)
{
	(new GC_Boom_Standard(hit + norm, GetOwner()))->_damage = DAMAGE_ROCKET_AK47;
	ApplyHitDamage(object, hit);
	Kill();
	return true;
}

void GC_Rocket::SpawnTrailParticle(const vec2d &pos)
{
	static TextureCache fire1("particle_fire");
	static TextureCache fire2("particle_fire2");

	new GC_Particle(pos - GetDirection() * 8.0f,
		GetDirection() * (_velocity * 0.3f), _target ? fire2:fire1, frand(0.1f) + 0.02f);
}

void GC_Rocket::TimeStepFixed(float dt)
{
	_timeHomming += dt;

	if( _target )
	{
		if( _target->IsKilled() || WEAP_RL_HOMMING_TIME < _timeHomming )
		{
			FreeTarget();
		}
		else
		{
			vec2d target;
			g_level->CalcOutstrip(GetPos(), _velocity, _target->GetPos(), _target->_lv, target);

			vec2d a = target - GetPos();

			vec2d p = a - GetDirection();
			vec2d dv = p - GetDirection() * (GetDirection() * p);

			float ldv = dv.len();
			if( ldv > 0 )
			{
				dv /= (ldv * _velocity);
				dv *= WEAP_RL_HOMMING_FACTOR;

				vec2d dir(GetDirection());
				dir += dv * dt;
				dir.Normalize();
				SetDirection(dir);
			}
		}
	}

	GC_Projectile::TimeStepFixed(dt);
}

void GC_Rocket::FreeTarget()
{
	_target = NULL;
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Bullet)
{
	return true;
}

GC_Bullet::GC_Bullet(const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(ignore, owner, advanced, true, x, v, /*"projectile_bullet"*/ NULL)
  , _trailEnable(false)
{
	SetHitDamage(advanced ? DAMAGE_BULLET * 2 : DAMAGE_BULLET);
	SetHitImpulse(5);
	SetTrailDensity(5.0f);

	SetVisible(false);
	_light->SetActive(false);
}

GC_Bullet::GC_Bullet(FromFile)
  : GC_Projectile(FromFile())
{
}

GC_Bullet::~GC_Bullet()
{
}

void GC_Bullet::Serialize(SaveFile &f)
{
	GC_Projectile::Serialize(f);
	f.Serialize(_trailEnable);
}

bool GC_Bullet::OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth)
{
	static TextureCache tex("particle_trace");

	//
	// spawn particles
	//
	float n = norm.Angle();
	float a1 = n - 1.4f;
	float a2 = n + 1.4f;
	for( int i = 0; i < 7; ++i )
	{
		vec2d a(a1 + frand(a2 - a1));
		new GC_Particle(hit, a * (frand(50.0f) + 50.0f), tex, frand(0.1f) + 0.03f, a);
	}

	GC_Light *pLight = new GC_Light(GC_Light::LIGHT_POINT);
	pLight->MoveTo(hit);
	pLight->SetRadius(50);
	pLight->SetIntensity(0.5f);
	pLight->SetTimeout(0.3f);

	ApplyHitDamage(object, hit);
	Kill();
	return true;
}

void GC_Bullet::SpawnTrailParticle(const vec2d &pos)
{
	static const TextureCache tex("particle_trace2");

	if( !(rand() & (_trailEnable ? 0x1f : 0x7F)) )
	{
		_trailEnable = !_trailEnable;
		_light->SetActive(_trailEnable);
	}

	if( _trailEnable )
	{
		new GC_Particle(pos, vec2d(0,0), tex, frand(0.01f) + 0.09f, GetDirection());
	}
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_TankBullet)
{
	return true;
}

GC_TankBullet::GC_TankBullet(const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(ignore, owner, advanced, TRUE, x, v, "projectile_cannon")
{
	SetTrailDensity(5.0f);
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

bool GC_TankBullet::OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth)
{
	static TextureCache tex1("particle_trace");
	static TextureCache tex2("explosion_s");

	if( GetAdvanced() )
	{
		(new GC_Boom_Big( vec2d(__max(0, __min(g_level->_sx - 1, hit.x + norm.x)),
								__max(0, __min(g_level->_sy - 1, hit.y + norm.y))),
						  GetOwner() ))->_time_boom = 0.05f;
	}
	else
	{
		float a = norm.Angle();
		float a1 = a - 1.4f;
		float a2 = a + 1.4f;
		for( int n = 0; n < 9; n++ )
		{
			vec2d a(a1 + frand(a2 - a1));
			new GC_Particle(hit, a * (frand(100.0f) + 50.0f), tex1, frand(0.2f) + 0.05f, a);
		}

		GC_Light *pLight = new GC_Light(GC_Light::LIGHT_POINT);
		pLight->MoveTo(hit);
		pLight->SetRadius(80);
		pLight->SetIntensity(1.5f);
		pLight->SetTimeout(0.3f);

		new GC_Particle(hit, vec2d(0,0), tex2, 0.3f, vrand(1));
		PLAY(SND_BoomBullet, hit);
	}

	ApplyHitDamage(object, hit);
	Kill();
	return true;
}

void GC_TankBullet::SpawnTrailParticle(const vec2d &pos)
{
	static TextureCache tex1("particle_trace");
	static TextureCache tex2("particle_trace2");
	new GC_Particle(pos, vec2d(0,0), GetAdvanced() ? tex1:tex2, frand(0.05f) + 0.05f, GetDirection());
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_PlazmaClod)
{
	return true;
}

GC_PlazmaClod::GC_PlazmaClod(const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(ignore, owner, advanced, TRUE, x, v, "projectile_plazma")
{
	SetHitDamage(DAMAGE_PLAZMA);
	SetTrailDensity(4.0f);

	PLAY(SND_PlazmaFire, GetPos());
}

GC_PlazmaClod::GC_PlazmaClod(FromFile)
  : GC_Projectile(FromFile())
{
}

GC_PlazmaClod::~GC_PlazmaClod()
{
}

bool GC_PlazmaClod::OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth)
{
	static TextureCache tex1("particle_green");
	static TextureCache tex2("explosion_plazma");

	if( GetAdvanced() && !object->IsKilled() )
	{
		new GC_HealthDaemon(object, GetOwner(), 15.0f, 2.0f);
	}

	float a = norm.Angle();
	float a1 = a - 1.5f;
	float a2 = a + 1.5f;
	for( int n = 0; n < 15; n++ )
	{
		vec2d a(a1 + frand(a2 - a1));
		new GC_Particle(hit, a * (frand(100.0f) + 50.0f), tex1, frand(0.2f) + 0.05f, a);
	}

	GC_Light *pLight = new GC_Light(GC_Light::LIGHT_POINT);
	pLight->MoveTo(hit);
	pLight->SetRadius(90);
	pLight->SetIntensity(1.5f);
	pLight->SetTimeout(0.4f);

	new GC_Particle( hit, vec2d(0,0), tex2, 0.3f, vrand(1));
	PLAY(SND_PlazmaHit, hit);

	ApplyHitDamage(object, hit);
	Kill();
	return true;
}

void GC_PlazmaClod::SpawnTrailParticle(const vec2d &pos)
{
	static TextureCache tex1("particle_green");
	new GC_Particle(pos, vec2d(0,0), tex1, frand(0.15f) + 0.10f);
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_BfgCore)
{
	return true;
}

GC_BfgCore::GC_BfgCore(const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(ignore, owner, advanced, TRUE, x, v, "projectile_bfg")
  , _time(0)
{
	PLAY(SND_BfgFire, GetPos());

	SetTrailDensity(2.5f);
	SetHitDamage(DAMAGE_BFGCORE);

	FindTarget();

	_light->SetRadius(WEAP_BFG_RADIUS * 2);
}

GC_BfgCore::GC_BfgCore(FromFile)
  : GC_Projectile(FromFile())
{
}

GC_BfgCore::~GC_BfgCore()
{
}

void GC_BfgCore::FindTarget()
{
	GC_RigidBodyDynamic *pNearestTarget = NULL; // by angle
	float nearest_cosinus = 0;

	FOREACH( g_level->GetList(LIST_vehicles), GC_RigidBodyDynamic, veh )
	{
		if( veh->IsKilled() || GetOwner() == veh->GetOwner() ) continue;

		// проверка видимости цели
		if( veh != g_level->TraceNearest(g_level->grid_rigid_s,
			GetIgnore(), GetPos(), veh->GetPos() - GetPos()) ) continue;

		vec2d a = veh->GetPos() - GetPos();

		// косинус угла направления на цель
		float cosinus = (a * GetDirection()) / a.len();

		if( cosinus > nearest_cosinus )
		{
			nearest_cosinus = cosinus;
			pNearestTarget = veh;
		}
	}

	// выбираем только если ближе 30 градусов
	if( nearest_cosinus > 0.87f )
		_target = pNearestTarget;
}

void GC_BfgCore::Kill()
{
	_target = NULL;
	GC_Projectile::Kill();
}

void GC_BfgCore::Serialize(SaveFile &f)
{
	GC_Projectile::Serialize(f);
	f.Serialize(_time);
	f.Serialize(_target);
}

bool GC_BfgCore::OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth)
{
	static TextureCache tex1("particle_green");
	static TextureCache tex2("explosion_g");

	float a = norm.Angle();
	float a1 = a - 1.4f;
	float a2 = a + 1.4f;
	for(int n = 0; n < 64; n++)
	{
		//ring
		new GC_Particle(hit, vec2d(a1 + frand(a2 - a1)) * (frand(100.0f) + 50.0f),
						tex1, frand(0.3f) + 0.15f);
	}


	GC_Light *pLight = new GC_Light(GC_Light::LIGHT_POINT);
	pLight->MoveTo(hit);
	pLight->SetRadius(WEAP_BFG_RADIUS * 3);
	pLight->SetIntensity(1.5f);
	pLight->SetTimeout(0.5f);

	new GC_Particle( hit, vec2d(0,0), tex2, 0.3f );
	PLAY(SND_BfgFlash, hit);

	ApplyHitDamage(object, hit);
	Kill();
	return true;
}

void GC_BfgCore::SpawnTrailParticle(const vec2d &pos)
{
	static TextureCache tex("particle_green");
	vec2d dx = vrand(WEAP_BFG_RADIUS) * frand(1.0f);
	new GC_Particle(pos + dx, vrand(7.0f), tex, 0.7f);
}

void GC_BfgCore::TimeStepFixed(float dt)
{
	_time += dt;
	if( _time * ANIMATION_FPS >= 1 )
	{
		SetFrame( (GetCurrentFrame() + 1)  % GetFrameCount() );
		_time -= 1/ANIMATION_FPS;
		FindTarget();
	}

	FOREACH( g_level->GetList(LIST_vehicles), GC_RigidBodyDynamic, veh )
	{
		if( !veh->IsKilled() )
		{
			const float R = WEAP_BFG_RADIUS;
			float damage = (1 - (GetPos() - veh->GetPos()).len() / R) *
				(fabs(veh->_lv.len()) / SPEED_BFGCORE * 10 + 0.5f);

			if( damage > 0 && !(GetAdvanced() && GetOwner() == veh->GetOwner()) )
			{
				vec2d delta(GetPos() - veh->GetPos());
				delta.Normalize();
				vec2d d = delta + g_level->net_vrand(1.0f);
				veh->TakeDamage(damage * DAMAGE_BFGCORE * dt, veh->GetPos() + d, GetOwner());
			}
		}
	}

	//------------------------------------------

	if( _target )
	{
		if( _target->IsKilled() )
		{
			_target = NULL;
		}
		else
		{
			vec2d target;
			g_level->CalcOutstrip(GetPos(), _velocity, _target->GetPos(), _target->_lv, target);

			vec2d a = target - GetPos();

			vec2d p = a - GetDirection();
			vec2d dv = p - GetDirection() * (GetDirection() * p);

			float ldv = dv.len();
			if( ldv > 0 )
			{
				dv /= (ldv * _velocity);
				dv *= (3.0f * fabs(_target->_lv.len()) / WEAP_BFG_TARGET_SPEED + GetAdvanced() ? 1 : 0) * WEAP_BFG_HOMMING_FACTOR;

				vec2d dir(GetDirection());
				dir += dv * dt;
				dir.Normalize();
				SetDirection(dir);
			}
		}
	}

	GC_Projectile::TimeStepFixed(dt);
}


/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_FireSpark)
{
	return true;
}

GC_FireSpark::GC_FireSpark(const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(ignore, owner, advanced, TRUE, x, v, "projectile_fire" )
  , _time(0)
  , _timeLife(1)
  , _rotation(frand(10) - 5)
{
	SetHitDamage(DAMAGE_FIRE_HIT);
	SetTrailDensity(4.5f);
	SetFrame(rand() % GetFrameCount());
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

void GC_FireSpark::Serialize(SaveFile &f)
{
	GC_Projectile::Serialize(f);
	f.Serialize(_time);
	f.Serialize(_timeLife);
	f.Serialize(_rotation);
}

void GC_FireSpark::Draw() const
{
	vec2d pos = GetPosPredicted();
	float r = GetRadius();
	g_texman->DrawSprite(GetTexture(), GetCurrentFrame(), GetColor(), pos.x, pos.y, r, r, GetDirection());
}

bool GC_FireSpark::OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth)
{
	assert(!object->IsKilled());

	vec2d nn(norm.y, -norm.x);

	if( GetDirection() * nn < (g_level->net_frand(0.6f) - 0.3f) )
	{
		nn = -nn;
		_rotation = frand(4) + 5;
	}
	else
	{
		_rotation = -frand(4) - 5;
	}


	float vdotn = GetDirection() * norm;
	float up = (0.5f - 0.6f * g_level->net_frand(vdotn));
	up *= up;
	up *= up;
	up *= up;
	up *= up;

	nn += norm * up;
	nn.Normalize();

	SetDirection(nn);
	_velocity *= 0.9f / (1 + up * 4);

	if( GetAdvanced() && GetOwner() != object->GetOwner()
		&& (g_level->net_rand()&1) && CheckFlags(GC_FLAG_FIRESPARK_SETFIRE) )
	{
		new GC_HealthDaemon(object, GetOwner(), 10.0f, 3.0f);
	}

	ApplyHitDamage(object, hit);

	if( GC_RigidBodyStatic *tmp = g_level->TraceNearest(g_level->grid_rigid_s, object, hit, norm) )
	{
		if( tmp->GetOwner() != GetOwner() )
		{
			ApplyHitDamage(tmp, hit);
			Kill();
		}
	}
	else
	{
		MoveTo(hit + norm, true);
	}
	return true;
}

void GC_FireSpark::SpawnTrailParticle(const vec2d &pos)
{
	static TextureCache tex("projectile_fire");
	
	if( g_conf.g_particles.Get() )
	{
		GC_Particle *p = new GC_ParticleScaled(pos + vrand(3), 
			GetDirection() * (_velocity/3) + vrand(10.0f), tex, 0.1f + frand(0.3f), vrand(1), GetRadius());
		p->SetFade(true);
		p->SetAutoRotate(_rotation);
	}

	// random walk
	vec2d tmp = GetDirection() + vec2d(GetDirection().y, -GetDirection().x) * (g_level->net_frand(0.06f) - 0.03f);
	SetDirection(tmp.Normalize());
}

float GC_FireSpark::FilterDamage(float damage, GC_RigidBodyStatic *object)
{
	assert(!object->IsKilled());
	if( GetAdvanced() && GetOwner() != object->GetOwner() )
	{
		return CheckFlags(GC_FLAG_FIRESPARK_HEALOWNER) ? -damage : 0;
	}
	return damage;
}

void GC_FireSpark::TimeStepFixed(float dt)
{
	float R = GetRadius();
	_light->SetRadius(3*R);

	R *= 1.5; // for damage calculation

	PtrList<ObjectList> receive;
	g_level->grid_rigid_s.OverlapPoint(receive, GetPos() / LOCATION_SIZE);

	const bool healOwner = CheckFlags(GC_FLAG_FIRESPARK_HEALOWNER);

	PtrList<ObjectList>::iterator it1 = receive.begin();
	for( ; it1 != receive.end(); ++it1 )
	{
		FOREACH_SAFE(**it1, GC_RigidBodyStatic, object)
		{
			if( object->CheckFlags(GC_FLAG_RBSTATIC_PHANTOM|GC_FLAG_OBJECT_KILLED) )
			{
				continue;
			}

			vec2d dist = GetPos() - object->GetPos();
			float destLen = dist.len();

			float damage = (1 - destLen / R) * DAMAGE_FIRE * dt;
			if( damage > 0 )
			{
				if( GetAdvanced() && GetOwner() == object->GetOwner() )
				{
					if( healOwner )
					{
						object->SetHealthCur(__min(object->GetHealth() + damage, object->GetHealthMax()));
					}
				}
				else
				{
					vec2d d = dist.Normalize() + g_level->net_vrand(1.0f);
					object->TakeDamage(damage, object->GetPos() + d, GetOwner());
				}
			}
		}
	}

	_time += dt;
	if( _time > _timeLife )
	{
		Kill();
	}
	else
	{
		SafePtr<GC_FireSpark> refHolder;

		// this moves the particle by velocity*dt
		GC_Projectile::TimeStepFixed(dt);

		// correct particle's position as if it was affected by air friction
		if( !IsKilled() )
		{
			const float a = 1.5;
			float e = exp(-a * dt);
			vec2d correcton = GetDirection() * (_velocity * ((1 - e) / a - dt));
			_velocity *= e;
			MoveTo(GetPos() + correcton, false);
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

GC_ACBullet::GC_ACBullet(const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(ignore, owner, advanced, TRUE, x, v, "projectile_ac")
{
	SetHitDamage(DAMAGE_ACBULLET);
	SetHitImpulse(20);
	SetTrailDensity(5.0f);
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

bool GC_ACBullet::OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth)
{
	static const TextureCache tex("particle_trace");

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
		new GC_Particle(hit, dir * frand(300.0f), tex, frand(0.05f) + 0.05f, dir);
	}

	GC_Light *pLight = new GC_Light(GC_Light::LIGHT_POINT);
	pLight->MoveTo(hit + norm * 5.0f);
	pLight->SetRadius(80);
	pLight->SetIntensity(1.5f);
	pLight->SetTimeout(0.1f);

	ApplyHitDamage(object, hit);
	Kill();
	return true;
}

void GC_ACBullet::SpawnTrailParticle(const vec2d &pos)
{
	static const TextureCache tex("particle_trace2");
	new GC_Particle(pos, vec2d(0,0), tex, frand(0.05f) + 0.05f, GetDirection());
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_GaussRay)
{
	return true;
}

GC_GaussRay::GC_GaussRay(const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(ignore, owner, advanced, true, x, v, NULL)
{
	SetHitDamage(DAMAGE_GAUSS);
	SetHitImpulse(100);
	SetTrailDensity(16.0f);

	PLAY(SND_Bolt, GetPos());

	SAFE_KILL(_light);

	_light = WrapRawPtr(new GC_Light(GC_Light::LIGHT_DIRECT));
	_light->SetRadius(64);
	_light->SetLength(0);
	_light->SetIntensity(1.5f);
	vec2d d = -v;
	d.Normalize();
	_light->SetLightDirection(d);
	_light->MoveTo(GetPos());

	SetShadow(false);
	SetZ(Z_NONE);
}

GC_GaussRay::GC_GaussRay(FromFile)
  : GC_Projectile(FromFile())
{
}

GC_GaussRay::~GC_GaussRay()
{
}

void GC_GaussRay::SpawnTrailParticle(const vec2d &pos)
{
	static const TextureCache tex1("particle_gauss1");
	static const TextureCache tex2("particle_gauss2");

	const TextureCache *t = &tex1;

	if( GetAdvanced() )
	{
		t = &tex2;
//		(new GC_Particle(pos + vrand(4), GetDirection() * (_velocity * 0.01f), tex3, 0.3f, GetDirection()))->SetFade(true);
	}

	GC_Particle *p = new GC_Particle(pos, vec2d(0,0), *t, 0.2f, GetDirection());
	p->SetZ(Z_GAUSS_RAY);
	p->SetFade(true);

	_light->SetLength(_light->GetLength() + GetTrailDensity());
}

bool GC_GaussRay::OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth)
{
	static const TextureCache tex("particle_gausshit");
	(new GC_Particle(hit, vec2d(0,0), tex, 0.5f, vec2d(norm.y, -norm.x)))->SetFade(true);

	//ApplyHitDamage(object, hit);
	if( GC_RigidBodyDynamic *dyn = dynamic_cast<GC_RigidBodyDynamic *>(object) )
	{
		dyn->ApplyImpulse(GetDirection() * GetHitDamage() / DAMAGE_GAUSS * 100, hit);
	}
	float damage = FilterDamage(GetHitDamage(), object);
	if( damage >= 0 )
	{
		object->TakeDamage(damage * relativeDepth, hit, GetOwner());
	}


	if( GetAdvanced() )
		relativeDepth /= 4;
	SetHitDamage(GetHitDamage() - relativeDepth * DAMAGE_GAUSS_FADE);
	SetHitImpulse(GetHitDamage() / DAMAGE_GAUSS * 100);
	if( GetHitDamage() <= 0 )
	{
		MoveTo(hit, CheckFlags(GC_FLAG_PROJECTILE_TRAIL)); // workaround to see trail at last step
		Kill();
		return true;
	}
	return false; // don't stop
}

void GC_GaussRay::Kill()
{
	if( _light && !_light->IsKilled() ) // _light can be killed during level cleanup
		_light->SetTimeout(0.4f);
	GC_Projectile::Kill();
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Disk)
{
	return true;
}

GC_Disk::GC_Disk(const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced)
  : GC_Projectile(ignore, owner, advanced, true, x, v, "projectile_disk")
{
	SetHitDamage(g_level->net_frand(DAMAGE_DISK_MAX - DAMAGE_DISK_MIN) + DAMAGE_DISK_MIN * (advanced ? 2.0f : 1.0f));
	SetHitImpulse(GetHitDamage() / DAMAGE_DISK_MAX * 20);
	SetTrailDensity(5.0f);
	_light->SetActive(false);
}

GC_Disk::GC_Disk(FromFile)
  : GC_Projectile(FromFile())
{
}

GC_Disk::~GC_Disk()
{
}

bool GC_Disk::OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth)
{
	static const TextureCache tex1("particle_trace");
	static const TextureCache tex2("explosion_e");

	ApplyHitDamage(object, hit);

	SetDirection(GetDirection() - norm * 2 * (GetDirection() * norm));
	MoveTo(hit + norm, true);

	for( int i = 0; i < 11; ++i )
	{
		vec2d v = (norm + vrand(frand(1.0f))) * 100.0f;
		vec2d vnorm = v;
		vnorm.Normalize();
		new GC_Particle(hit, v, tex1, frand(0.2f) + 0.02f, vnorm);
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
			new GC_Bullet(
				GetPos(),
				vec2d(a1 + g_level->net_frand(a2 - a1)) * (g_level->net_frand(2000.0f) + 3000.0f),
				GetIgnore(),
				GetOwner(),
				GetAdvanced());
		}

		new GC_Particle(hit, vec2d(0,0), tex2, 0.2f, vrand(1));

		GC_Light *pLight = new GC_Light(GC_Light::LIGHT_POINT);
		pLight->MoveTo(hit);
		pLight->SetRadius(100);
		pLight->SetIntensity(1.5f);
		pLight->SetTimeout(0.2f);

		PLAY(SND_BoomBullet, hit);
		Kill();
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
			new GC_Bullet(
				GetPos(),
				vec2d(a1 + g_level->net_frand(a2 - a1)) * (g_level->net_frand(2000.0f) + 3000.0f),
				GetIgnore(),
				GetOwner(),
				true);
		}
	}

	GC_Light *pLight = new GC_Light(GC_Light::LIGHT_POINT);
	pLight->MoveTo(hit + norm * 5.0f);
	pLight->SetRadius(70);
	pLight->SetIntensity(1.5f);
	pLight->SetTimeout(0.1f);

	return true;
}

void GC_Disk::SpawnTrailParticle(const vec2d &pos)
{
	static const TextureCache tex("particle_trace2");

	vec2d dx = vrand(3.0f);
	float time = frand(0.01f) + 0.03f;

	vec2d v = (-dx - GetDirection() * (-dx * GetDirection())) / time;
	vec2d dir(v - GetDirection() * (32.0f / time));
	dir.Normalize();
	new GC_Particle(pos + dx - GetDirection()*4.0f, v, tex, time, dir);
}

float GC_Disk::FilterDamage(float damage, GC_RigidBodyStatic *object)
{
	assert(!object->IsKilled());
	if( GetAdvanced() && GetOwner() == object->GetOwner() )
	{
		return damage / 3; // one third of damage to owner
	}
	return damage;
}

///////////////////////////////////////////////////////////////////////////////
// end of file
