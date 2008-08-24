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
#include "RigidBody.h"
#include "sound.h"
#include "vehicle.h"
#include "particles.h"
#include "pickup.h"

///////////////////////////////////////////////////////////////////////////////

GC_Projectile::GC_Projectile(GC_RigidBodyStatic *owner, bool advanced, bool trail,
                             const vec2d &pos, const vec2d &v, const char *texture)
  : GC_2dSprite()
  , _memberOf(this)
  , _light(new GC_Light(GC_Light::LIGHT_POINT))
  , _owner(owner)
  , _hitDamage(0)
{
	SetZ(Z_PROJECTILE);
	SetShadow(true);

	SetFlags(GC_FLAG_PROJECTILE_IGNOREOWNER);
	if( advanced ) SetFlags(GC_FLAG_PROJECTILE_ADVANCED);
	if( trail )    SetFlags(GC_FLAG_PROJECTILE_TRAIL);

	_trailDensity = 10.0f;
	_trailPath    = 0.0f;

	_velocity = v;
	_hitImpulse  = 0;


	SetRotation( v.Angle() );

	SetTexture(texture);
	MoveTo(pos, FALSE);

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

void GC_Projectile::SpecialTrace(GC_RigidBodyDynamic *pObj, const vec2d &path)
{
	if( CheckFlags(GC_FLAG_PROJECTILE_IGNOREOWNER) && _owner == pObj )
	{
		return;
	}

	vec2d norm;
	vec2d m[4];

	float min_len = path.len();
	bool  hit     = false;

	for( int i = 0; i < 4; ++i )
	{
		m[i] = pObj->GetVertex(i);
	}

	for( int n = 0; n < 4; ++n )
	{
		float xb = m[n].x;
		float yb = m[n].y;

		float bx = m[(n+1)&3].x - xb;
		float by = m[(n+1)&3].y - yb;

		float delta = path.y*bx - path.x*by;
		if( delta <= 0 ) continue;

		float tb = (path.x*(yb - GetPos().y) - path.y*(xb - GetPos().x)) / delta;

		if( tb <= 1 && tb >= 0 )
		{
			float len = (bx*(yb - GetPos().y) - by*(xb - GetPos().x)) / delta;
			if( len >= 0 && len <= min_len )
			{
				min_len = len;
				hit     = true;
				norm.x  =  by;
				norm.y  = -bx;
			}
		}
	}

	if( hit )
	{
		norm.Normalize();
		if( Hit(pObj, GetPos(), norm) )
		{
			Kill();
			return;
		}
	}
}

void GC_Projectile::Kill()
{
	_owner   = NULL;
	_lastHit = NULL;
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
	f.Serialize(_lastHit);
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

void GC_Projectile::TimeStepFixed(float dt)
{
	GC_2dSprite::TimeStepFixed(dt);

	vec2d dx = _velocity * dt;

	vec2d hit, norm;
	GC_RigidBodyStatic *object = g_level->agTrace(
		g_level->grid_rigid_s,
		_lastHit ? GetRawPtr(_lastHit) : (
			CheckFlags(GC_FLAG_PROJECTILE_IGNOREOWNER) ? GetRawPtr(_owner) : NULL
		), GetPos(), dx, &hit, &norm
	);

	if( object )
	{
#ifdef _DEBUG
		vec2d pos = GetPos();
#endif
		if( Hit(object, hit, norm) )
		{
			_ASSERT(GetPos() == pos);
			Kill();
			return;
		}
		else
		{
			_ASSERT(GetPos() == pos);
			float new_dt = dt * (1.0f - sqrtf((hit - GetPos()).sqr() / dx.sqr()));
			if( new_dt > 1e-6 )
			{
				MoveTo(hit, CheckFlags(GC_FLAG_PROJECTILE_TRAIL));
				TimeStepFixed(new_dt);
			}
		}
		return;
	}
	else
	{
		if( _lastHit )
		{
			_lastHit->Unsubscribe(this);
			_lastHit = NULL;
		}
		MoveTo(GetPos() + dx, CheckFlags(GC_FLAG_PROJECTILE_TRAIL));
	}

	if( GetPos().x < 0 || GetPos().x > g_level->_sx ||
		GetPos().y < 0 || GetPos().y > g_level->_sy )
	{
		Kill();
		return;
	}
}

bool GC_Projectile::Hit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm)
{
	_ASSERT(_lastHit != object);
	if( _lastHit )
	{
		_lastHit->Unsubscribe(this);
		_lastHit = NULL;
	}
	object->AddRef();
	if( GC_RigidBodyDynamic *dyn = dynamic_cast<GC_RigidBodyDynamic *>(object) )
	{
		vec2d tmp = _velocity;
		tmp.Normalize();
		dyn->ApplyImpulse(tmp * _hitImpulse, hit);
	}
	float damage = FilterDamage(_hitDamage, object);
	if( damage >= 0 )
	{
		object->TakeDamage(damage, hit, GetRawPtr(_owner));
	}
	else
	{
		// heal
		object->SetHealthCur(__min(object->GetHealth() - damage, object->GetHealthMax()));
	}
	bool result = OnHit(object, hit, norm);
	if( !IsKilled() && !object->IsKilled() )
	{
		_ASSERT(NULL == _lastHit);
		_lastHit = object;
		_lastHit->Subscribe(NOTIFY_OBJECT_KILL, this, (NOTIFYPROC) &GC_Projectile::OnKillLastHit);
	}
	object->Release();
	return result;
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


void GC_Projectile::OnKillLastHit(GC_Object *sender, void *param)
{
	_lastHit = NULL;
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Rocket)
{
	return true;
}

GC_Rocket::GC_Rocket(const vec2d &x, const vec2d &v, GC_RigidBodyStatic* owner, bool advanced)
  : GC_Projectile(owner, advanced, TRUE, x, v, "projectile_rocket")
  , _timeHomming(0.0f)
{
	SetTrailDensity(1.5f);
	SetHitImpulse(15);

	new GC_Sound_link(SND_RocketFly, SMODE_LOOP, this);

	// advanced rocket searches for target
	if( GetAdvanced() )
	{
		GC_Vehicle *pNearestVehicle = NULL; // ближайшее по углу
		float nearest_cosinus = 0;

		FOREACH( g_level->GetList(LIST_vehicles), GC_Vehicle, veh )
		{
			if( veh->IsKilled() || _owner == veh )
				continue;

			// проверка видимости цели
			if( veh != g_level->agTrace(g_level->grid_rigid_s,
				GetRawPtr(_owner), GetPos(), veh->GetPos() - GetPos()) )
			{
				continue;
			}


			vec2d target;
			g_level->CalcOutstrip(GetPos(), _velocity.len(), veh->GetPos(), veh->_lv, target);

			vec2d a = target - GetPos();

			// косинус угла направления на цель
			float cosinus = (a * _velocity) / (a.len() * _velocity.len());

			if( cosinus > nearest_cosinus )
			{
				nearest_cosinus = cosinus;
				pNearestVehicle = veh;
			}
		}

		// выбираем только если ближе 20 градусов
		if( nearest_cosinus > 0.94f )
		{
			_target = pNearestVehicle;
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

bool GC_Rocket::OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm)
{
	(new GC_Boom_Standard(hit + norm, GetRawPtr(_owner)))->_damage = DAMAGE_ROCKET_AK47;
	return true;
}

void GC_Rocket::SpawnTrailParticle(const vec2d &pos)
{
	static TextureCache fire1("particle_fire");
	static TextureCache fire2("particle_fire2");

	if( _target )
	{
		new GC_Particle(pos - _velocity * 8.0f / _velocity.len(),
			_velocity * 0.3f, fire2, frand(0.1f) + 0.02f);
	}
	else
	{
		new GC_Particle(pos - _velocity * 8.0f / _velocity.len(),
			_velocity * 0.3f, fire1, frand(0.1f) + 0.02f);
	}
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
			g_level->CalcOutstrip(GetPos(), _velocity.len(), _target->GetPos(), _target->_lv, target);

			vec2d a = target - GetPos();

			vec2d vi(_velocity);
			vi.Normalize();

			vec2d p = a - vi;
			vec2d dv = p - vi * (vi * p);

			float ldv = dv.len();
			if( ldv > 0 )
			{
				dv /= ldv;
				dv *= WEAP_RL_HOMMING_FACTOR;

				_velocity += dv * dt /* + vi * dt * WEAP_RL_HOMMING_FACTOR*/;
				SetRotation( _velocity.Angle() );
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

GC_Bullet::GC_Bullet(const vec2d &x, const vec2d &v, GC_RigidBodyStatic* owner, bool advanced)
  : GC_Projectile(owner, advanced, TRUE, x, v, /*"projectile_bullet"*/ NULL)
  , _trailEnable(false)
{
	SetHitDamage(advanced ? DAMAGE_BULLET * 2 : DAMAGE_BULLET);
	SetHitImpulse(5);
	SetTrailDensity(5.0f);

	Show(false);
	_light->Activate(false);
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

bool GC_Bullet::OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm)
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
		float a = a1 + frand(a2 - a1);
		new GC_Particle(hit, vec2d(a) * (frand(50.0f) + 50.0f), tex, frand(0.1f) + 0.03f, a);
	}

	GC_Light *pLight = new GC_Light(GC_Light::LIGHT_POINT);
	pLight->MoveTo(hit);
	pLight->SetRadius(50);
	pLight->SetIntensity(0.5f);
	pLight->SetTimeout(0.3f);

	return true;
}

void GC_Bullet::SpawnTrailParticle(const vec2d &pos)
{
	static const TextureCache tex("particle_trace2");

	if( !(rand() & (_trailEnable ? 0x1f : 0x7F)) )
	{
		_trailEnable = !_trailEnable;
		_light->Activate(_trailEnable);
	}

	if( _trailEnable )
	{
		new GC_Particle(pos, vec2d(0,0), tex, frand(0.01f) + 0.09f, _velocity.Angle());
	}
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_TankBullet)
{
	return true;
}

GC_TankBullet::GC_TankBullet(const vec2d &x, const vec2d &v, GC_RigidBodyStatic* owner, bool advanced)
  : GC_Projectile(owner, advanced, TRUE, x, v, "projectile_cannon")
{
	SetTrailDensity(5.0f);
	SetHitDamage(DAMAGE_TANKBULLET);
	SetHitImpulse(100);
	_light->Activate(advanced);
	PLAY(SND_Shoot, GetPos());
}

GC_TankBullet::GC_TankBullet(FromFile)
  : GC_Projectile(FromFile())
{
}

GC_TankBullet::~GC_TankBullet()
{
}

bool GC_TankBullet::OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm)
{
	static TextureCache tex1("particle_trace");
	static TextureCache tex2("explosion_s");

	if( GetAdvanced() )
	{
		(new GC_Boom_Big( vec2d(__max(0, __min(g_level->_sx - 1, hit.x + norm.x)),
								__max(0, __min(g_level->_sy - 1, hit.y + norm.y))),
						  GetRawPtr(_owner) ))->_time_boom = 0.05f;
	}
	else
	{
		float a = norm.Angle();
		float a1 = a - 1.4f;
		float a2 = a + 1.4f;
		for( int n = 0; n < 9; n++ )
		{
			float a = a1 + frand(a2 - a1);
			new GC_Particle(hit, vec2d(a) * (frand(100.0f) + 50.0f), tex1, frand(0.2f) + 0.05f, a);
		}

		GC_Light *pLight = new GC_Light(GC_Light::LIGHT_POINT);
		pLight->MoveTo(hit);
		pLight->SetRadius(80);
		pLight->SetIntensity(1.5f);
		pLight->SetTimeout(0.3f);

		(new GC_Particle( hit, vec2d(0,0), tex2, 0.3f ))->SetRotation(frand(PI2));
		PLAY(SND_BoomBullet, hit);
	}

	return true;
}

void GC_TankBullet::SpawnTrailParticle(const vec2d &pos)
{
	static TextureCache tex1("particle_trace");
	static TextureCache tex2("particle_trace2");

	if( GetAdvanced() )
	{
		new GC_Particle(pos, vec2d(0,0), tex1,
			frand(0.05f) + 0.05f, _velocity.Angle());
	}
	else
	{
		new GC_Particle(pos, vec2d(0,0), tex2,
			frand(0.05f) + 0.05f, _velocity.Angle());
	}
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_PlazmaClod)
{
	return true;
}

GC_PlazmaClod::GC_PlazmaClod(const vec2d &x, const vec2d &v, GC_RigidBodyStatic* owner, bool advanced)
  : GC_Projectile(owner, advanced, TRUE, x, v, "projectile_plazma")
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

bool GC_PlazmaClod::OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm)
{
	static TextureCache tex1("particle_green");
	static TextureCache tex2("explosion_plazma");

	if( GetAdvanced() && !object->IsKilled() )
	{
		new GC_HealthDaemon(object, GetRawPtr(_owner), 15.0f, 2.0f);
	}

	float a = norm.Angle();
	float a1 = a - 1.5f;
	float a2 = a + 1.5f;
	for( int n = 0; n < 15; n++ )
	{
		float a = a1 + frand(a2 - a1);
		new GC_Particle(hit, vec2d(a) * (frand(100.0f) + 50.0f), tex1, frand(0.2f) + 0.05f, a);
	}

	GC_Light *pLight = new GC_Light(GC_Light::LIGHT_POINT);
	pLight->MoveTo(hit);
	pLight->SetRadius(90);
	pLight->SetIntensity(1.5f);
	pLight->SetTimeout(0.4f);


	(new GC_Particle( hit, vec2d(0,0), tex2, 0.3f ))->SetRotation(frand(PI2));
	PLAY(SND_PlazmaHit, hit);

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

GC_BfgCore::GC_BfgCore(const vec2d &x, const vec2d &v, GC_RigidBodyStatic* owner, bool advanced)
  : GC_Projectile(owner, advanced, TRUE, x, v, "projectile_bfg")
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
	GC_Vehicle *pNearestVehicle = NULL; // ближайшее по углу
	float nearest_cosinus = 0;

	FOREACH( g_level->GetList(LIST_vehicles), GC_Vehicle, veh )
	{
		if( veh->IsKilled() || _owner == veh ) continue;

		// проверка видимости цели
		if( veh != g_level->agTrace(g_level->grid_rigid_s,
			GetRawPtr(_owner), GetPos(), veh->GetPos() - GetPos()) ) continue;

		vec2d target;
		g_level->CalcOutstrip(GetPos(), _velocity.len(), veh->GetPos(), veh->_lv, target);

		vec2d a = target - GetPos();

		// косинус угла направления на цель
		float cosinus = (a * _velocity) / (a.len() * _velocity.len());

		if( cosinus > nearest_cosinus )
		{
			nearest_cosinus = cosinus;
			pNearestVehicle = veh;
		}
	}

	// выбираем только если ближе 30 градусов
	if( nearest_cosinus > 0.87f )
		_target = pNearestVehicle;
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

bool GC_BfgCore::OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm)
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

	FOREACH( g_level->GetList(LIST_vehicles), GC_Vehicle, veh )
	{
		if( !veh->IsKilled() )
		{
			const float R = WEAP_BFG_RADIUS;
			float damage = (1 - (GetPos() - veh->GetPos()).len() / R) *
				(fabsf(veh->_lv.len()) / SPEED_BFGCORE * 10 + 0.5f);

			if( damage > 0 && !(GetAdvanced() && veh == _owner) )
			{
				vec2d d = (GetPos() - veh->GetPos()).Normalize() + g_level->net_vrand(1.0f);
				veh->TakeDamage(damage * DAMAGE_BFGCORE * dt, veh->GetPos() + d, GetRawPtr(_owner));
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
			float v = _velocity.len();

			vec2d target;
			g_level->CalcOutstrip(GetPos(), v, _target->GetPos(), _target->_lv, target);

			vec2d a = target - GetPos();

			vec2d vi = _velocity / v;

			vec2d p = a - vi;
			vec2d dv = p - vi * (vi * p);

			float ldv = dv.len();
			if( ldv > 0 )
			{
				dv /= ldv;
				dv *= (3.0f * fabsf(_target->_lv.len()) /
					_target->GetMaxSpeed() +
					GetAdvanced() ? 1 : 0) * WEAP_BFG_HOMMING_FACTOR;

				_velocity += dv * dt;
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

GC_FireSpark::GC_FireSpark(const vec2d &x, const vec2d &v, GC_RigidBodyStatic* owner, bool advanced)
  : GC_Projectile(owner, advanced, TRUE, x, v, "projectile_fire" )
  , _time(0)
  , _timeLife(1)
  , _rotation(frand(10) - 5)
  , _healOwner(false)
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
	f.Serialize(_healOwner);
}

bool GC_FireSpark::OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm)
{
	SetIgnoreOwner(false); // allow hit owner next time

	vec2d nn(norm.y, -norm.x);

	float v = _velocity.len();

	if( (_velocity * nn) / v < g_level->net_frand(0.6f) - 0.3f )
	{
		nn = -nn;
		_rotation = frand(4) + 5;
	}
	else
	{
		_rotation = -frand(4) - 5;
	}


	float vdotn = (_velocity * norm) / v;
	float up = (0.5f - 0.6f * g_level->net_frand(vdotn));
	up *= up;
	up *= up;
	up *= up;
	up *= up;

	nn += norm * up;
	nn.Normalize();

	nn /= (1 + up * 4);

	_velocity = nn * (v * 0.9f);

	if( GetAdvanced() && !object->IsKilled() && _owner != object && (g_level->net_rand()&1)
		&& CheckFlags(GC_FLAG_FIRESPARK_SETFIRE) )
	{
		new GC_HealthDaemon(object, GetRawPtr(_owner), 10.0f, 3.0f);
	}

	return false;
}

void GC_FireSpark::SpawnTrailParticle(const vec2d &pos)
{
	static TextureCache tex("projectile_fire");
	
	if( g_conf->g_particles->Get() )
	{
		GC_Particle *p = new GC_Particle(pos + vrand(3), _velocity/3 + vrand(10.0f), tex, 0.1f + frand(0.3f), frand(PI2));
		p->SetFade(true);
		p->SetAutoRotate(_rotation);
		p->Resize(GetRadius(), GetRadius());
		p->CenterPivot();
	}

	vec2d dv(_velocity.y, -_velocity.x);
	dv.Normalize();
	dv *= g_level->net_frand(20) - 10;
	_velocity += dv;
}

float GC_FireSpark::FilterDamage(float damage, GC_RigidBodyStatic *object)
{
	if( GetAdvanced() && _owner == object )
	{
		return _healOwner ? -damage : 0;
	}
	return damage;
}

void GC_FireSpark::TimeStepFixed(float dt)
{
	_time += dt;

	if( _time > _timeLife )
	{
		Kill();
		return;
	}


	float R = GetRadius();

	_light->SetRadius(3*R);

	SetRotation(GetRotation() + _rotation * dt);
	Resize(R, R);
	CenterPivot();

	R *= 1.5; // for damage calculation

	std::vector<OBJECT_LIST*> receive;
	g_level->grid_rigid_s.OverlapCircle(receive,
		GetPos().x / LOCATION_SIZE, GetPos().y / LOCATION_SIZE, 0);

	std::vector<OBJECT_LIST*>::iterator it1 = receive.begin();
	for( ; it1 != receive.end(); ++it1 )
	{
		for( OBJECT_LIST::safe_iterator it2 = (*it1)->safe_begin(); it2 != (*it1)->end(); ++it2 )
		{
			GC_RigidBodyStatic *object = (GC_RigidBodyStatic *) (*it2);
			vec2d dist = GetPos() - object->GetPos();
			float destLen = dist.len();

			float damage = (1 - destLen / R) * DAMAGE_FIRE * dt;
			if( damage > 0 )
			{
				if( GetAdvanced() && object == _owner )
				{
					if( _healOwner )
					{
						_owner->SetHealthCur(__min(_owner->GetHealth() + damage, _owner->GetHealthMax()));
					}
				}
				else
				{
					vec2d d = dist.Normalize() + g_level->net_vrand(1.0f);
					object->TakeDamage(damage, object->GetPos() + d, GetRawPtr(_owner));
				}
			}
		}
	}

	GC_Projectile::TimeStepFixed(dt);
}

void GC_FireSpark::SetLifeTime(float t)
{
	_timeLife = t;
}

void GC_FireSpark::SetHealOwner(bool heal)
{
	_healOwner = heal;
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_ACBullet)
{
	return true;
}

GC_ACBullet::GC_ACBullet(const vec2d &x, const vec2d &v, GC_RigidBodyStatic* owner, bool advanced)
  : GC_Projectile(owner, advanced, TRUE, x, v, "projectile_ac")
{
	SetHitDamage(DAMAGE_ACBULLET);
	SetHitImpulse(20);
	SetTrailDensity(5.0f);
	_light->SetRadius(30);
	_light->SetIntensity(0.6f);
	_light->Activate(advanced);
}

GC_ACBullet::GC_ACBullet(FromFile)
  : GC_Projectile(FromFile())
{
}

GC_ACBullet::~GC_ACBullet()
{
}

bool GC_ACBullet::OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm)
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
		float ang = a1 + frand(a2 - a1);
		new GC_Particle(hit, vec2d(ang) * frand(300.0f), tex, frand(0.05f) + 0.05f, ang);
	}

	GC_Light *pLight = new GC_Light(GC_Light::LIGHT_POINT);
	pLight->MoveTo(hit + norm * 5.0f);
	pLight->SetRadius(80);
	pLight->SetIntensity(1.5f);
	pLight->SetTimeout(0.1f);

	return true;
}

void GC_ACBullet::SpawnTrailParticle(const vec2d &pos)
{
	static const TextureCache tex("particle_trace2");
	new GC_Particle(pos, vec2d(0,0), tex,
		frand(0.05f) + 0.05f, _velocity.Angle());
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_GaussRay)
{
	return true;
}

GC_GaussRay::GC_GaussRay(const vec2d &x, const vec2d &v, GC_RigidBodyStatic* owner, bool advanced)
  : GC_Projectile(owner, advanced, TRUE, x, v, NULL)
{
	SetHitDamage(DAMAGE_GAUSS);
	SetHitImpulse(100);
	SetTrailDensity(16.0f);

	PLAY(SND_Bolt, GetPos());

	SAFE_KILL(_light);

	_light = new GC_Light(GC_Light::LIGHT_DIRECT);
	_light->SetRadius(64);
	_light->SetLength(0);
	_light->SetIntensity(1.5f);
	_light->SetAngle(v.Angle() + PI);

	_light->MoveTo(GetPos());

	SetShadow(false);
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

	float a = _velocity.Angle();

	if( GetAdvanced() )
	{
		t = &tex2;
//		(new GC_Particle(pos + vrand(4), _velocity * 0.01f, tex3, 0.3f, a))->SetFade(true);
	}

	GC_Particle *p = new GC_Particle(pos, vec2d(0,0), *t, 0.4f, a);
	p->SetZ(Z_GAUSS_RAY);
	p->SetFade(true);

	_light->SetLength(_light->GetLength() + GetTrailDensity());
}

bool GC_GaussRay::OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm)
{
	static const TextureCache tex("particle_gausshit");
	(new GC_Particle(hit, vec2d(0,0), tex, 0.5f, norm.Angle() + PI*0.5f))->SetFade(true);;
	SetHitDamage(GetHitDamage() - GetAdvanced() ? DAMAGE_GAUSS_FADE/4 : DAMAGE_GAUSS_FADE);
	SetHitImpulse(GetHitDamage() / DAMAGE_GAUSS * 100);
	return (0 >= GetHitDamage());
}

void GC_GaussRay::Kill()
{
	if( !_light->IsKilled() ) // _light can be killed during level cleanup
		_light->SetTimeout(0.4f);
	GC_Projectile::Kill();
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Disk)
{
	return true;
}

GC_Disk::GC_Disk(const vec2d &x, const vec2d &v, GC_RigidBodyStatic* owner, bool advanced)
  : GC_Projectile(owner, advanced, TRUE, x, v, "projectile_disk")
{
	SetHitDamage(g_level->net_frand(DAMAGE_DISK_MAX - DAMAGE_DISK_MIN) + DAMAGE_DISK_MIN * (advanced ? 2.0f : 1.0f));
	SetHitImpulse(GetHitDamage() / DAMAGE_DISK_MAX * 20);
	SetTrailDensity(5.0f);
	_light->Activate(false);
	SetRotation(frand(PI2));
}

GC_Disk::GC_Disk(FromFile)
  : GC_Projectile(FromFile())
{
}

GC_Disk::~GC_Disk()
{
}

bool GC_Disk::OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm)
{
	static const TextureCache tex1("particle_trace");
	static const TextureCache tex2("explosion_e");

	SetIgnoreOwner(false); // allow hit owner next time


	_velocity -= norm * 2 * (_velocity * norm);
	for( int i = 0; i < 11; ++i )
	{
		vec2d v = (norm + vrand(frand(1.0f))) * 100.0f;
		new GC_Particle(hit, v, tex1, frand(0.2f) + 0.02f, v.Angle());
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
			(new GC_Bullet(
				hit,
				vec2d(a1 + g_level->net_frand(a2 - a1)) * (g_level->net_frand(2000.0f) + 3000.0f),
				GetRawPtr(_owner),
				GetAdvanced())
			)->SetIgnoreOwner(false);
		}

		(new GC_Particle( hit, vec2d(0,0), tex2, 0.2f ))->SetRotation(frand(PI2));

		GC_Light *pLight = new GC_Light(GC_Light::LIGHT_POINT);
		pLight->MoveTo(hit);
		pLight->SetRadius(100);
		pLight->SetIntensity(1.5f);
		pLight->SetTimeout(0.2f);

		PLAY(SND_BoomBullet, hit);
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
			(new GC_Bullet(
				hit,
				vec2d(a1 + g_level->net_frand(a2 - a1)) * (g_level->net_frand(2000.0f) + 3000.0f),
				GetRawPtr(_owner),
				true)
			)->SetIgnoreOwner(false);
		}
	}

	GC_Light *pLight = new GC_Light(GC_Light::LIGHT_POINT);
	pLight->MoveTo(hit + norm * 5.0f);
	pLight->SetRadius(70);
	pLight->SetIntensity(1.5f);
	pLight->SetTimeout(0.1f);

	return false;
}

void GC_Disk::SpawnTrailParticle(const vec2d &pos)
{
	static const TextureCache tex("particle_trace2");

	vec2d dx = vrand(3.0f);
	vec2d ve = _velocity / _velocity.len();
	float time = frand(0.01f) + 0.03f;

	vec2d v = (-dx - ve * (-dx * ve)) / time;
	new GC_Particle(pos + dx - ve*4.0f, v, tex, time, (v - ve * (32.0f / time)).Angle());
}

float GC_Disk::FilterDamage(float damage, GC_RigidBodyStatic *object)
{
	if( GetAdvanced() && _owner == object )
	{
		return damage / 2; // half damage to owner
	}
	return damage;
}

void GC_Disk::TimeStepFixed(float dt)
{
	SetRotation( GetRotation() + dt * 10.0f );
	GC_Projectile::TimeStepFixed(dt);
}

///////////////////////////////////////////////////////////////////////////////
// end of file
