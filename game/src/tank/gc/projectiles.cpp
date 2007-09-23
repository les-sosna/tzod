// projectiles.cpp
////////////////////////////////////////////////////////

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
: GC_2dSprite(), _memberOf(this)
{
	SetZ(Z_PROJECTILE);
	SetShadow(true);

	SetFlags(GC_FLAG_PROJECTILE_IGNOREOWNER);
	if( advanced )	SetFlags(GC_FLAG_PROJECTILE_ADVANCED);
	if( trail )    SetFlags(GC_FLAG_PROJECTILE_TRAIL);

	_trailDensity = 10.0f;
	_trailPath    = 0.0f;

	_velocity = v;

	_light      = new GC_Light(GC_Light::LIGHT_POINT);
	_owner = owner;

	SetRotation( v.Angle() );

	SetTexture(texture);
	MoveTo(pos, FALSE);

	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FIXED);
}

GC_Projectile::GC_Projectile(FromFile)
  : GC_2dSprite(FromFile()), _memberOf(this)
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
		_lastHit = pObj;
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
	_owner = NULL;
	_lastHit    = NULL;
	if( _light )
	{
		if( _light->GetTimeout() > 0 )
			_light = NULL;
		else
			SAFE_KILL(_light);
	}
	/////////////
	GC_2dSprite::Kill();
}

void GC_Projectile::Serialize(SaveFile &f)
{
	GC_2dSprite::Serialize(f);
	/////////////////////////////////////
	f.Serialize(_damage);
	f.Serialize(_trailDensity);
	f.Serialize(_trailPath);
	f.Serialize(_velocity);
	f.Serialize(_light);
	f.Serialize(_owner);
	f.Serialize(_lastHit);
}

void GC_Projectile::MoveTo(const vec2d &pos, BOOL trail)
{
	if( trail )
	{
		vec2d e = pos - GetPos();
		float len = e.len();

		e /= len;

		while( _trailPath < len )
		{
			if( g_conf.g_particles->Get() )
				SpawnTrailParticle(GetPos() + e * _trailPath);
			_trailPath += _trailDensity;
		}

		_trailPath -= len;
	}
	else
	{
		_trailPath = frand(_trailDensity);
	}

	_light->MoveTo(pos);
	GC_2dSprite::MoveTo(pos);
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
		),	GetPos(), dx, &hit, &norm
	);

	if( object )
	{
		if( _lastHit )
		{
			_lastHit->Unsubscribe(this);
		}
		_lastHit = object;
		_lastHit->Subscribe(NOTIFY_OBJECT_KILL, this, (NOTIFYPROC) &GC_Projectile::OnKillLastHit);

		if( Hit(object, hit, norm) )
		{
			MoveTo(hit, CheckFlags(GC_FLAG_PROJECTILE_TRAIL));
			Kill();
		}
		else
		{
			float new_dt = dt * (1.0f - sqrtf((hit - GetPos()).sqr() / dx.sqr()));
			MoveTo(hit, CheckFlags(GC_FLAG_PROJECTILE_TRAIL));
			TimeStepFixed(new_dt);
		}
		return;
	}
	else
	{
		MoveTo(GetPos() + dx, CheckFlags(GC_FLAG_PROJECTILE_TRAIL));
	}

	if( GetPos().x < 0 || GetPos().x > g_level->_sx ||
		GetPos().y < 0 || GetPos().y > g_level->_sy )
	{
		Kill();
		return;
	}
}

void GC_Projectile::OnKillLastHit(GC_Object *sender, void *param)
{
	_lastHit = NULL;
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Rocket)
{
	return true;
}

GC_Rocket::GC_Rocket(const vec2d &x, const vec2d &v, GC_RigidBodyStatic* owner, bool advanced)
: GC_Projectile(owner, advanced, TRUE, x, v, "projectile_rocket")
{
	_damage = DAMAGE_ROCKET_AK47;
	_trailDensity = 1.5f;

	_timeHomming = 0.0f;
	_owner = owner;

	new GC_Sound_link(SND_RocketFly, SMODE_LOOP, this);

	// для кваженой ракеты производится поиск цели
	if( IsAdvanced() )
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

GC_Rocket::GC_Rocket(FromFile) : GC_Projectile(FromFile())
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

bool GC_Rocket::Hit(GC_Object *object, const vec2d &hit, const vec2d &norm)
{
	(new GC_Boom_Standard(hit + norm, GetRawPtr(_owner)))->_damage = _damage;
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
{
	_damage = DAMAGE_BULLET;
	_trailDensity = 5.0f;

	_path = 0;
	_path_trail_on = frand(400.0f) + frand(500.0f) + frand(600.0f);
	_path_trail_off = _path_trail_on + frand(50.0f) + frand(50.0f) + frand(50.0f);

	Show(false);
	_light->Activate(false);
}

GC_Bullet::GC_Bullet(FromFile) : GC_Projectile(FromFile())
{
}

GC_Bullet::~GC_Bullet()
{
}

void GC_Bullet::Serialize(SaveFile &f)
{
	GC_Projectile::Serialize(f);
	f.Serialize(_path);
	f.Serialize(_path_trail_on);
	f.Serialize(_path_trail_off);
}

bool GC_Bullet::Hit(GC_Object *object, const vec2d &hit, const vec2d &norm)
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


	// кваженый патрон не повреждает хозяина
//	if( !(IsAdvanced() && (object == _owner)) )
	{
		((GC_RigidBodyStatic*) object)->TakeDamage(_damage, hit, GetRawPtr(_owner));
	}

	return true;
}

void GC_Bullet::SpawnTrailParticle(const vec2d &pos)
{
	static const TextureCache tex("particle_trace2");

	_path += _trailDensity;

	if( _path > _path_trail_off )
	{
		_path_trail_on = _path_trail_off + frand(100.2f) + frand(100.2f) + frand(100.2f) + frand(100.2f) + frand(100.2f);
		_path_trail_off = _path_trail_on + frand(10.1f) + frand(10.1f) + frand(10.1f) + frand(10.1f);
		ClearFlags(GC_FLAG_PROJECTILE_TRAIL);
		_light->Activate(false);
	}
	else if( _path > _path_trail_on )
	{
		_light->Activate(true);
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
	_damage        = DAMAGE_TANKBULLET;
	_trailDensity = 5.0f;
	_light->Activate(advanced);
	PLAY(SND_Shoot, GetPos());
}

GC_TankBullet::GC_TankBullet(FromFile) : GC_Projectile(FromFile())
{
}

GC_TankBullet::~GC_TankBullet()
{
}

bool GC_TankBullet::Hit(GC_Object *object, const vec2d &hit, const vec2d &norm)
{
	static TextureCache tex1("particle_trace");
	static TextureCache tex2("explosion_s");

	if( GC_RigidBodyDynamic *dyn = dynamic_cast<GC_RigidBodyDynamic *>(object) )
	{
		dyn->ApplyImpulse(-norm * (_damage / DAMAGE_GAUSS) * 100, hit);
	}

	((GC_RigidBodyStatic*) object)->TakeDamage(_damage, hit, GetRawPtr(_owner));

	if( IsAdvanced() )
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

	if( IsAdvanced() )
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
	_damage        = DAMAGE_PLAZMA;
	_trailDensity = 4.0f;

	PLAY(SND_PlazmaFire, GetPos());
}

GC_PlazmaClod::GC_PlazmaClod(FromFile) : GC_Projectile(FromFile())
{
}

GC_PlazmaClod::~GC_PlazmaClod()
{
}

bool GC_PlazmaClod::Hit(GC_Object *object, const vec2d &hit, const vec2d &norm)
{
	static TextureCache tex1("particle_green");
	static TextureCache tex2("explosion_plazma");

	if( IsAdvanced() )
	{
		new GC_HealthDaemon(((GC_RigidBodyStatic*) object),
			GetRawPtr(_owner), 15.0f, 2.0f);
	}


	((GC_RigidBodyStatic*) object)->TakeDamage(_damage, hit, GetRawPtr(_owner));

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
{
	_time = 0;
	_damage = DAMAGE_BFGCORE;
	PLAY(SND_BfgFire, GetPos());
	_trailDensity = 2.5f;

	FindTarget();

	_light->SetRadius(WEAP_BFG_RADIUS * 2);
}

GC_BfgCore::GC_BfgCore(FromFile) : GC_Projectile(FromFile())
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

bool GC_BfgCore::Hit(GC_Object *object, const vec2d &hit, const vec2d &norm)
{
	static TextureCache tex1("particle_green");
	static TextureCache tex2("explosion_g");

	((GC_RigidBodyStatic*) object)->TakeDamage(_damage, hit, GetRawPtr(_owner));

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

			if( damage > 0 && !(IsAdvanced() && veh == _owner) )
			{
				vec2d d = (GetPos() - veh->GetPos()).Normalize() + g_level->net_vrand(1.0f);
				veh->TakeDamage(damage * _damage * dt,
					veh->GetPos() + d, GetRawPtr(_owner));
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
					IsAdvanced() ? 1 : 0) * WEAP_BFG_HOMMING_FACTOR;

				_velocity += dv * dt;
			}
		}
	}

	GC_Projectile::TimeStepFixed(dt);
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_ACBullet)
{
	return true;
}

GC_ACBullet::GC_ACBullet(const vec2d &x, const vec2d &v, GC_RigidBodyStatic* owner, bool advanced)
: GC_Projectile(owner, advanced, TRUE, x, v, "projectile_ac")
{
	_damage = DAMAGE_ACBULLET;
	_trailDensity = 5;
	_light->SetRadius(30);
	_light->SetIntensity(0.6f);
	_light->Activate(advanced);
}

GC_ACBullet::GC_ACBullet(FromFile) : GC_Projectile(FromFile())
{
}

GC_ACBullet::~GC_ACBullet()
{
}

bool GC_ACBullet::Hit(GC_Object *object, const vec2d &hit, const vec2d &norm)
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

	((GC_RigidBodyStatic*) object)->TakeDamage(_damage, hit, GetRawPtr(_owner));


	float a = norm.Angle();
	float a1 = a - 1.0f;
	float a2 = a + 1.0f;
	for(int i = 0; i < 12; i++)
	{
		float ang = a1 + frand(a2 - a1);
		new GC_Particle(hit, vec2d(ang) * frand(300.0f),
			tex, frand(0.05f) + 0.05f, ang);
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
	_damage = DAMAGE_GAUSS;
	_trailDensity = 16.0f;

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

GC_GaussRay::GC_GaussRay(FromFile) : GC_Projectile(FromFile())
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

	if( IsAdvanced() )
	{
		t = &tex2;
//		(new GC_Particle(pos + vrand(4), _velocity * 0.01f, tex3, 0.3f, a))->SetFade(true);
	}

	GC_Particle *p = new GC_Particle(pos, vec2d(0,0), *t, 0.4f, a);
	p->SetZ(Z_GAUSS_RAY);
	p->SetFade(true);

	_light->SetLength(_light->GetLength() + _trailDensity);
}

bool GC_GaussRay::Hit(GC_Object *object, const vec2d &hit, const vec2d &norm)
{
	static const TextureCache tex("particle_gausshit");

	if( GC_RigidBodyDynamic *dyn = dynamic_cast<GC_RigidBodyDynamic *>(object) )
	{
		dyn->ApplyImpulse(-norm * (_damage / DAMAGE_GAUSS) * 100, hit);
	}

	((GC_RigidBodyStatic*) object)->TakeDamage(_damage, hit, GetRawPtr(_owner));

	(new GC_Particle(hit, vec2d(0,0), tex, 0.5f, norm.Angle() + PI*0.5f))->SetFade(true);;

	_damage -= IsAdvanced() ? DAMAGE_GAUSS_FADE/4 : DAMAGE_GAUSS_FADE;

	return (0 >= _damage);
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

//GC_Disk::GC_Disk(GC_Weap_Ripper *pRipper)
//: GC_Projectile((GC_RigidBodyStatic *) NULL, false, FALSE, pRipper->GetPos(), vec2d(0, 0), "projectile_disk")
//{
//	_damage = 0;
//
//	_time = g_level->net_frand(1.0f);
//
//	_attached = true;
//	_ripper = pRipper;
//	_ripper->Subscribe(NOTIFY_OBJECT_KILL, this,
//		(NOTIFYPROC) &GC_Disk::OnRipperKill, true, false);
//	_ripper->Subscribe(NOTIFY_ACTOR_MOVE, this,
//		(NOTIFYPROC) &GC_Disk::OnRipperMove, false, false);
//
//	_light->Activate(false);
//}

GC_Disk::GC_Disk(const vec2d &x, const vec2d &v, GC_RigidBodyStatic* owner, bool advanced)
: GC_Projectile(owner, advanced, TRUE, x, v, "projectile_disk")
{
	_damage = g_level->net_frand(DAMAGE_DISK_MAX - DAMAGE_DISK_MIN) + DAMAGE_DISK_MIN * (advanced ? 2.0f : 1.0f);
	_trailDensity = 5.0f;
	_time = g_level->net_frand(1.0f);
//	_attached = false;
	_light->Activate(false);
}

GC_Disk::GC_Disk(FromFile) : GC_Projectile(FromFile())
{
}

GC_Disk::~GC_Disk()
{
}

void GC_Disk::Serialize(SaveFile &f)
{
	GC_Projectile::Serialize(f);
//	f.Serialize(_attached);
	f.Serialize(_time);
//	f.Serialize(_ripper);
}

bool GC_Disk::Hit(GC_Object *object, const vec2d &hit, const vec2d &norm)
{
	static const TextureCache tex1("particle_trace");
	static const TextureCache tex2("explosion_e");

//	if( _attached ) return false;

	ClearFlags(GC_FLAG_PROJECTILE_IGNOREOWNER);

	if( object == _owner )
	{
	//	if( !IsAdvanced() )
		{
			((GC_RigidBodyStatic*) object)->TakeDamage(
				_damage * 0.5f, hit, GetRawPtr(_owner));
		}
	}
	else
		((GC_RigidBodyStatic*) object)->TakeDamage(_damage, hit, GetRawPtr(_owner));
	_velocity -= norm * 2 * (_velocity * norm);
	MoveTo(hit + norm, CheckFlags(GC_FLAG_PROJECTILE_TRAIL));
	for( int i = 0; i < 11; ++i )
	{
		vec2d v = (norm + vrand(frand(1.0f))) * 100.0f;
		new GC_Particle(hit, v,	tex1, frand(0.2f) + 0.02f, v.Angle());
	}

	_damage -= DAMAGE_DISK_FADE;
	if( _damage <= 0 )
	{
		float a = norm.Angle();

		float a1 = a - PI / 3;
		float a2 = a + PI / 3;

		for( int n = 0; n < 14; ++n )
		{
			(new GC_Bullet(
				GetPos(), 
				vec2d(a1 + g_level->net_frand(a2 - a1)) * (g_level->net_frand(2000.0f) + 3000.0f),
				GetRawPtr(_owner), 
				IsAdvanced())
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
	if( IsAdvanced() )
	{
		float a = norm.Angle();

		float a1 = a - PI / 4;
		float a2 = a + PI / 4;

		for( int n = 0; n < 11; ++n )
		{
			(new GC_Bullet(
				GetPos(), 
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

//void GC_Disk::Kill()
//{
//	if( _attached )
//	{
//		_ripper   = NULL;
//		_attached = false;
//	}
//
//	GC_Projectile::Kill();
//}

void GC_Disk::TimeStepFixed(float dt)
{
	_time += dt;
	SetRotation( _time * 10.0f );

	//if( _attached )
	//{
	//	if( _ripper->_attached )
	//	{
	//		Show(_ripper->_time >= _ripper->_timeReload);
	//	}
	//	else
	//		Show(false);
	//}
	//else
		GC_Projectile::TimeStepFixed(dt);
}

//void GC_Disk::OnRipperMove(GC_Object *sender, void *param)
//{
//	GC_Weap_Ripper *r = (GC_Weap_Ripper *) sender;
//	if( r->_owner )
//		MoveTo(r->GetPos() - (vec2d(r->_owner->_angle + r->_angle) * 7), FALSE);
//}
//
//void GC_Disk::OnRipperKill(GC_Object *sender, void *param)
//{
//	Kill();
//}

///////////////////////////////////////////////////////////////////////////////
// end of file
