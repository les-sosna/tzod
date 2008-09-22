// Turrets.h

#include "stdafx.h"
#include "Turrets.h"

#include "macros.h"
#include "level.h"
#include "functions.h"

#include "core/JobManager.h"
#include "core/debug.h"

#include "fs/MapFile.h"
#include "fs/SaveFile.h"

#include "GameClasses.h"
#include "sound.h"
#include "indicators.h"
#include "vehicle.h"
#include "player.h"
#include "projectiles.h"
#include "particles.h"

//////////////////////////////////////////////////////////////////////////////////////////////

JobManager<GC_Turret> GC_Turret::_jobManager;

GC_Turret::GC_Turret(float x, float y)
  : GC_RigidBodyStatic()
  , _rotator(_dir)
{
	SetZ(Z_WALLS);

	_team   = 0;
	_sight  = TURET_SIGHT_RADIUS;

	_initialDir = 0;

	_jobManager.RegisterMember(this);
	_state = TS_WAITING;
	_rotator.reset(0, 0, 2.0f, 5.0f, 10.0f);

	_rotateSound = new GC_Sound(SND_TuretRotate, SMODE_STOP, GetPos());
	_weaponSprite = new GC_UserSprite();
	_weaponSprite->SetShadow(true);
	_weaponSprite->SetZ(Z_FREE_ITEM);

	MoveTo(vec2d(x, y)); // this also moves _rotateSound and _weaponSprite

	new GC_IndicatorBar("indicator_health", this, &_health, &_health_max, LOCATION_TOP);

	///////////////////////
	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FIXED);
}

GC_Turret::GC_Turret(FromFile)
  : GC_RigidBodyStatic(FromFile())
  , _rotator(_dir)
{
}

void GC_Turret::Serialize(SaveFile &f)
{
	GC_RigidBodyStatic::Serialize(f);

	_rotator.Serialize(f);

	f.Serialize(_dir);
	f.Serialize(_initialDir);
	f.Serialize(_sight);
	f.Serialize(_state);
	f.Serialize(_team);
	f.Serialize(_rotateSound);
	f.Serialize(_target);
	f.Serialize(_weaponSprite);

	if( f.loading() && (TS_WAITING == _state || TS_HIDDEN == _state) )
	{
		_jobManager.RegisterMember(this);
	}
}

void GC_Turret::Kill()
{
	SAFE_KILL(_rotateSound);
	SAFE_KILL(_weaponSprite);
	_target = NULL;

	if( TS_WAITING == _state || TS_HIDDEN == _state )
	{
		_jobManager.UnregisterMember(this);
	}

	GC_RigidBodyStatic::Kill();
}

GC_Vehicle* GC_Turret::EnumTargets()
{
	float min_dist = _sight*_sight;
	float dist;

	GC_Vehicle *target = NULL;
	GC_RigidBodyStatic *pObstacle = NULL;

	FOREACH( g_level->GetList(LIST_vehicles), GC_Vehicle, pDamObj )
	{
		if( !pDamObj->GetPlayer() )
		{
			continue;
		}
		if( pDamObj->GetPlayer()->GetTeam() && pDamObj->GetPlayer()->GetTeam() == _team )
		{
			continue;
		}

		if( !pDamObj->IsKilled() )
		{
			dist = (GetPos().x - pDamObj->GetPos().x)*(GetPos().x - pDamObj->GetPos().x)
				 + (GetPos().y - pDamObj->GetPos().y)*(GetPos().y - pDamObj->GetPos().y);

			if( dist < min_dist && IsTargetVisible(pDamObj, &pObstacle) )
			{
				target = pDamObj;
				min_dist = dist;
			}
		}
	}

	return target;
}

void GC_Turret::SelectTarget(GC_Vehicle* target)
{
	_jobManager.UnregisterMember(this);
	_target = target;
	_state   = TS_ATACKING;
	PLAY(SND_TargetLock, GetPos());
}

void GC_Turret::TargetLost()
{
	_jobManager.RegisterMember(this);
	_target = NULL;
	_state  = TS_WAITING;
}

bool GC_Turret::IsTargetVisible(GC_Vehicle* target, GC_RigidBodyStatic** pObstacle)
{
	GC_RigidBodyStatic *object = g_level->agTrace(
		g_level->grid_rigid_s, this, GetPos(), target->GetPos() - GetPos());

	if( object != target )
	{
		*pObstacle = object;
		return false;
	}

	*pObstacle = NULL;
	return true;
}

void GC_Turret::MoveTo(const vec2d &pos)
{
	_rotateSound->MoveTo(pos);
	_weaponSprite->MoveTo(pos);
	GC_RigidBodyStatic::MoveTo(pos);
}

void GC_Turret::OnDestroy()
{
	new GC_Boom_Big( GetPos(), NULL);
	GC_RigidBodyStatic::OnDestroy();
}

void GC_Turret::TimeStepFixed(float dt)
{
	_rotator.process_dt(dt);
	_weaponSprite->SetRotation(_dir);

	switch( _state )
	{
	case TS_WAITING:
		if( _jobManager.TakeJob(this) )
		{
			GC_Vehicle *target;
			if( target = EnumTargets() )
				SelectTarget(target);
		}
		break;
	case TS_ATACKING:
	{
		GC_RigidBodyStatic *pObstacle = NULL;
		if( !_target->IsKilled() )
		{
			if( IsTargetVisible(GetRawPtr(_target), &pObstacle) )
			{
				float RotSpeed = 0;

				vec2d fake; // координаты мнимой цели
				CalcOutstrip(GetRawPtr(_target), fake);

				float ang2 = ( fake - GetPos() ).Angle();

				_rotator.rotate_to(ang2);

				float d1 = fabsf(ang2-_dir);
				float d2 = _dir < ang2 ? _dir-ang2+PI2 : ang2-_dir+PI2;

				if( __min(d1, d2) < (PI / 36.0) )
				{
					Fire();
				}
			}
			else
				TargetLost();
		}
		else
			TargetLost();
		break;
	} // end case TS_ATACKING
	default:
		_ASSERT(FALSE);
	}  // end switch (_state)

	_rotator.setup_sound(GetRawPtr(_rotateSound));
}

void GC_Turret::EditorAction()
{
	_dir += PI2 / 16;
	_dir = fmodf(_dir, PI2);
	_initialDir = _dir;
	_weaponSprite->SetRotation(_dir);
}

void GC_Turret::mapExchange(MapFile &f)
{
	GC_RigidBodyStatic::mapExchange(f);

	MAP_EXCHANGE_FLOAT(sight_radius,  _sight, TURET_SIGHT_RADIUS);
	MAP_EXCHANGE_FLOAT(dir, _initialDir, 0);
	MAP_EXCHANGE_INT  (team, _team, 0);

	if( f.loading() )
	{
		if( _team > MAX_TEAMS-1 )
			_team = MAX_TEAMS-1;
		_dir = _initialDir;
		_weaponSprite->SetRotation(_dir);
	}
}

void GC_Turret::Draw()
{
	GC_RigidBodyStatic::Draw();

	if( g_level->_modeEditor )
	{
		const char* teams[MAX_TEAMS] = {"", "1", "2", "3", "4", "5"};
		_ASSERT(_team >= 0 && _team < MAX_TEAMS);
		g_level->DrawText(teams[_team], GetPos() - vec2d(CELL_SIZE, CELL_SIZE));
	}
}

PropertySet* GC_Turret::NewPropertySet()
{
	return new MyPropertySet(this);
}

GC_Turret::MyPropertySet::MyPropertySet(GC_Object *object)
  : BASE(object)
  , _propTeam(  ObjectProperty::TYPE_INTEGER, "team"  )
  , _propSight( ObjectProperty::TYPE_INTEGER, "sight" )
{
	_propTeam.SetIntRange(0, MAX_TEAMS - 1);
	_propSight.SetIntRange(0, 100);
}

int GC_Turret::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 2;
}

ObjectProperty* GC_Turret::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
		case 0: return &_propTeam;
		case 1: return &_propSight;
	}

	_ASSERT(FALSE);
	return NULL;
}

void GC_Turret::MyPropertySet::MyExchange(bool applyToObject)
{
	BASE::MyExchange(applyToObject);

	GC_Turret *tmp = static_cast<GC_Turret *>(GetObject());

	if( applyToObject )
	{
		tmp->_team  = _propTeam.GetIntValue();
		tmp->_sight = (float) (_propSight.GetIntValue() * CELL_SIZE);
	}
	else
	{
		_propTeam.SetIntValue(tmp->_team);
		_propSight.SetIntValue(int(tmp->_sight / CELL_SIZE + 0.5f));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_TurretRocket)
{
	ED_TURRET( "turret_rocket", "obj_turret_rocket" );
	return true;
}

GC_TurretRocket::GC_TurretRocket(float x, float y)
  : GC_Turret(x, y)
  , _timeReload(0)
{
	_weaponSprite->SetTexture("turret_rocket");
	SetTexture("turret_platform");
	AlignToTexture();

	g_level->_field.ProcessObject(this, true);

	SetHealth(GetDefaultHealth(), GetDefaultHealth());
}

GC_TurretRocket::GC_TurretRocket(FromFile)
  : GC_Turret(FromFile())
{
}

void GC_TurretRocket::Serialize(SaveFile &f)
{
	GC_Turret::Serialize(f);
	f.Serialize(_timeReload);
}

void GC_TurretRocket::CalcOutstrip(const GC_Vehicle *target, vec2d &fake)
{
	g_level->CalcOutstrip(GetPos(), SPEED_ROCKET, target->GetPos(), target->_lv, fake);
}

void GC_TurretRocket::Fire()
{
	if( _timeReload <= 0 )
	{
		vec2d a(_dir);
		(new GC_Rocket(GetPos() + a * 25.0f, a * SPEED_ROCKET, this, true))
			->SetHitDamage(g_level->net_frand(10.0f));
		_timeReload = TURET_ROCKET_RELOAD;
	}
}

void GC_TurretRocket::TimeStepFixed(float dt)
{
	GC_Turret::TimeStepFixed(dt);
	_timeReload -= dt;
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_TurretCannon)
{
	ED_TURRET( "turret_cannon", "obj_turret_cannon" );
	return true;
}

GC_TurretCannon::GC_TurretCannon(float x, float y)
  : GC_Turret(x, y)
  , _timeReload(0)
  , _time_smoke(0)
  , _time_smoke_dt(0)
{
	_weaponSprite->SetTexture("turret_cannon");
	SetTexture("turret_platform");
	AlignToTexture();

	g_level->_field.ProcessObject(this, true);
	SetHealth(GetDefaultHealth(), GetDefaultHealth());
}

GC_TurretCannon::GC_TurretCannon(FromFile)
  : GC_Turret(FromFile())
{
}

GC_TurretCannon::~GC_TurretCannon()
{
}

void GC_TurretCannon::Serialize(SaveFile &f)
{
	GC_Turret::Serialize(f);
	/////////////////////////////////////
	f.Serialize(_timeReload);
	f.Serialize(_time_smoke);
	f.Serialize(_time_smoke_dt);
}

void GC_TurretCannon::CalcOutstrip(const GC_Vehicle *target, vec2d &fake)
{
	g_level->CalcOutstrip(GetPos(), SPEED_TANKBULLET, target->GetPos(), target->_lv, fake);
}

void GC_TurretCannon::Fire()
{
	if( _timeReload <= 0 )
	{
		vec2d a(_dir);
		(new GC_TankBullet(
			GetPos() + a * 31.9f,
			a * SPEED_TANKBULLET + g_level->net_vrand(40),
			this,
			false )
		)->SetHitDamage(g_level->net_frand(10.0f) + 5.0f);
		_timeReload = TURET_CANON_RELOAD;
		_time_smoke  = 0.1f;
	}
}

void GC_TurretCannon::TimeStepFixed(float dt)
{
	static const TextureCache tex("particle_smoke");

	GC_Turret::TimeStepFixed(dt);
	_timeReload -= dt;

	if( _time_smoke > 0 )
	{
		_time_smoke -= dt;
		_time_smoke_dt += dt;
		for( ;_time_smoke_dt > 0; _time_smoke_dt -= 0.025f )
		{
			new GC_Particle(GetPos() + vec2d(_dir) * 33.0f,
				SPEED_SMOKE + vec2d(_dir) * 50, tex, frand(0.3f) + 0.2f);
		}
	}
}

////////////////////////////////////////////////////////////////////

GC_TurretBunker::GC_TurretBunker(float x, float y)
  : GC_Turret(x, y)
  , _time(0)
  , _time_wait_max(1.0f)
  , _time_wait(_time_wait_max)
  , _time_wake_max(0.5f)
  , _time_wake(0)  // hidden
{
	_rotator.setl(2.0f, 20.0f, 30.0f);
	_state = TS_HIDDEN;
	_weaponSprite->Show(false);
}

GC_TurretBunker::GC_TurretBunker(FromFile)
  : GC_Turret(FromFile())
{
}

void GC_TurretBunker::Serialize(SaveFile &f)
{
	GC_Turret::Serialize(f);
	/////////////////////////////////////
	f.Serialize(_delta_angle);
	f.Serialize(_time);
	f.Serialize(_time_wait);
	f.Serialize(_time_wait_max);
	f.Serialize(_time_wake);
	f.Serialize(_time_wake_max);
}

void GC_TurretBunker::mapExchange(MapFile &f)
{
	GC_Turret::mapExchange(f);
	if( f.loading() )
	{
		SetRotation(_initialDir);
	}
}

GC_TurretBunker::~GC_TurretBunker()
{
}

void GC_TurretBunker::WakeUp()
{
	_jobManager.UnregisterMember(this);
	_state = TS_WAKING_UP;
	PLAY(SND_TuretWakeUp, GetPos());
}

void GC_TurretBunker::WakeDown()
{
	_state = TS_PREPARE_TO_WAKEDOWN;
	_jobManager.UnregisterMember(this);
}

bool GC_TurretBunker::TakeDamage(float damage, const vec2d &hit, GC_RigidBodyStatic *from)
{
	if( _state != TS_HIDDEN )
	{
		return GC_Turret::TakeDamage(damage, hit, from);
	}

	if( rand() < 128 )
		PLAY(SND_Hit1, hit);
	else if( rand() < 128 )
		PLAY(SND_Hit3, hit);
	else if( rand() < 128 )
		PLAY(SND_Hit5, hit);

	return false;
}

void GC_TurretBunker::TimeStepFixed(float dt)
{
	_rotator.process_dt(dt);
	_weaponSprite->SetRotation(_dir);

	switch( _state )
	{
	case TS_ATACKING:
	{
		GC_RigidBodyStatic *pObstacle = NULL;

		if( !_target->IsKilled() )
		{
			if( IsTargetVisible(GetRawPtr(_target), &pObstacle) )
			{
				vec2d fake;
				CalcOutstrip(GetRawPtr(_target), fake);

				float ang2 = ( fake - GetPos() ).Angle();

				_rotator.rotate_to(ang2);

				float d1 = fabsf(ang2-_dir);
				float d2 = _dir < ang2 ? _dir-ang2+PI2 : ang2-_dir+PI2;

				if( __min(d1, d2) <= _delta_angle ) Fire();
			}
			else
			{
				TargetLost();
			}
		}
		else
		{
			TargetLost();
		}
		_time_wait = g_level->net_frand(_time_wait_max);
		break;
	} // end case TS_ATACKING

	case TS_WAITING:
		_time_wait -= dt;
		if( _time_wait <= 0 )
		{
			_time_wait = g_level->net_frand(_time_wait_max);
			WakeDown();
		}
		else
		{
		//	if( _jobManager.TakeJob(this) )
			{
				GC_Vehicle *target;
				if( target = EnumTargets() )
					SelectTarget(target);
			}
		}
		break;

	case TS_HIDDEN:
		if( _jobManager.TakeJob(this) )
		{
			if( EnumTargets() ) WakeUp();
		}
		break;

	case TS_WAKING_UP:
		_time_wake += dt;
		if( _time_wake >= _time_wake_max )
		{
			_time_wake = _time_wake_max;
			_state = TS_WAITING;
			_jobManager.RegisterMember(this);
			_weaponSprite->Show(true);
			SetFrame(GetFrameCount() - 1);
		}
		else
		{
			SetFrame(int( (float)(GetFrameCount() - 1) * _time_wake / _time_wake_max ));
		}
		break;

	case TS_WAKING_DOWN:
		_time_wake -= dt;
		if( _time_wake <= 0 )
		{
			_time_wake = 0;
			_state = TS_HIDDEN;
			SetFrame(0);
			_jobManager.RegisterMember(this);
		}
		else
			SetFrame(int( (float)(GetFrameCount() - 1) * _time_wake / _time_wake_max ));
		break;

	case TS_PREPARE_TO_WAKEDOWN:
		if( _initialDir != _dir || RS_STOPPED != _rotator.GetState() )
		{
			_rotator.rotate_to(_initialDir);
		}
		else
		{
			PLAY(SND_TuretWakeDown, GetPos());
			_state = TS_WAKING_DOWN;
			_weaponSprite->Show(false);
		}
		break;
	default:
		_ASSERT(0);
	} // end switch (_state);

	_rotator.setup_sound(GetRawPtr(_rotateSound));
}

void GC_TurretBunker::EditorAction()
{
	_initialDir += PI / 2;
	_initialDir = fmodf(_initialDir, PI2);
	_dir = _initialDir;
	SetRotation(_initialDir);
	_weaponSprite->SetRotation(_initialDir);
}

////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_TurretMinigun)
{
	ED_TURRET( "turret_minigun", "obj_turret_minigun" );
	return true;
}

GC_TurretMinigun::GC_TurretMinigun(float x, float y)
  : GC_TurretBunker(x, y)
  , _fireSound(new GC_Sound(SND_MinigunFire, SMODE_STOP, GetPos()))
{
	_delta_angle = 0.5f; // точность стрельбы
	_rotator.reset(0, 0, 6.0f, 21.0f, 36.0f);

	_time = 0;
	_firing = false;

	_time_wait_max = 1.0f;
	_time_wait     = _time_wait_max;
	_time_wake_max = 0.5f;

	_weaponSprite->SetTexture("turret_mg");
	SetTexture("turret_mg_wake");
	AlignToTexture();

	g_level->_field.ProcessObject(this, true);
	SetHealth(GetDefaultHealth(), GetDefaultHealth());
}

GC_TurretMinigun::GC_TurretMinigun(FromFile)
  : GC_TurretBunker(FromFile())
{
}

void GC_TurretMinigun::Serialize(SaveFile &f)
{
	GC_TurretBunker::Serialize(f);
	f.Serialize(_firing);
	f.Serialize(_time);
	f.Serialize(_fireSound);
}

void GC_TurretMinigun::Kill()
{
	SAFE_KILL(_fireSound);
	GC_TurretBunker::Kill();
}

GC_TurretMinigun::~GC_TurretMinigun()
{
}

void GC_TurretMinigun::CalcOutstrip(const GC_Vehicle *target, vec2d &fake)
{
	g_level->CalcOutstrip(GetPos(), SPEED_BULLET, target->GetPos(), target->_lv, fake);
}

void GC_TurretMinigun::Fire()
{
	_firing = true;
}

void GC_TurretMinigun::TimeStepFixed(float dt)
{
	static const TextureCache tex("particle_1");

	GC_TurretBunker::TimeStepFixed(dt);

	if( _firing )
	{
		ASSERT_TYPE(GetRawPtr(_fireSound), GC_Sound);
		_fireSound->Pause(false);

		_time += dt;

		for( ; _time > 0; _time -= 0.04f )
		{
			float ang = _dir + g_level->net_frand(0.1f) - 0.05f;
			vec2d a(_dir);
			new GC_Bullet(GetPos() + a * 31.9f, vec2d(ang) * SPEED_BULLET, this, false );
			new GC_Particle(GetPos() + a * 31.9f, a * (400 + frand(400.0f)), tex, frand(0.06f) + 0.03f);
		}

		_firing = false;
	}
	else
	{
		ASSERT_TYPE(GetRawPtr(_fireSound), GC_Sound);
		_fireSound->Pause(true);
	}
}

////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_TurretGauss)
{
	ED_TURRET( "turret_gauss", "obj_turret_gauss" );
	return true;
}

GC_TurretGauss::GC_TurretGauss(float x, float y)
  : GC_TurretBunker(x, y)
{
	_delta_angle = 0.03f; // точность стрельбы
	_rotator.reset(0, 0, 10.0f, 30.0f, 60.0f);

	_time = 0;
	_shotCount = 0;

	_time_wait_max = 0.10f;
	_time_wake_max = 0.45f;

	_weaponSprite->SetTexture("turret_gauss");
	SetTexture("turret_gauss_wake");
	AlignToTexture();

	g_level->_field.ProcessObject(this, true);
	SetHealth(GetDefaultHealth(), GetDefaultHealth());
}

GC_TurretGauss::GC_TurretGauss(FromFile)
  : GC_TurretBunker(FromFile())
{
}

void GC_TurretGauss::TargetLost()
{
	_shotCount = 0;
	_time      = 0;
	GC_TurretBunker::TargetLost();
}

void GC_TurretGauss::Serialize(SaveFile &f)
{
	GC_TurretBunker::Serialize(f);
	f.Serialize(_shotCount);
	f.Serialize(_time);
}

GC_TurretGauss::~GC_TurretGauss()
{
}

void GC_TurretGauss::CalcOutstrip(const GC_Vehicle *target, vec2d &fake)
{
	g_level->CalcOutstrip(GetPos(), SPEED_GAUSS, target->GetPos(), target->_lv, fake);
}

void GC_TurretGauss::Fire()
{
	if( 0 == _shotCount )
	{
		_time = 0;
	}

	if( _time >= (float) _shotCount * 0.2f )
	{
		float dy = _shotCount == 0 ? -7.0f : 7.0f;
		float c = cosf(_dir), s = sinf(_dir);

		new GC_GaussRay(vec2d(GetPos().x + c * 20.0f - dy * s,
		                      GetPos().y + s * 20.0f + dy * c),
		                vec2d(c, s) * SPEED_GAUSS, this, false );

		if( ++_shotCount == 2 )
		{
			TargetLost();
			WakeDown();
		}
	}
}

void GC_TurretGauss::TimeStepFixed(float dt)
{
	GC_TurretBunker::TimeStepFixed(dt);
	_time += dt;
}


// end of file
