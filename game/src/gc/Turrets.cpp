#include "TypeReg.h"
#include "inc/gc/Turrets.h"
#include "inc/gc/Explosion.h"
#include "inc/gc/Indicators.h"
#include "inc/gc/Vehicle.h"
#include "inc/gc/Player.h"
#include "inc/gc/Projectiles.h"
#include "inc/gc/Particles.h"
#include "inc/gc/TypeSystem.h"
#include "inc/gc/Macros.h"
#include "inc/gc/WeapCfg.h"
#include "inc/gc/World.h"
#include "inc/gc/WorldEvents.h"

#include "inc/gc/SaveFile.h"

#include "inc/gc/detail/JobManager.h"
#include <MapFile.h>


#define TURET_SIGHT_RADIUS   500
#define TURET_ROCKET_RELOAD  0.9f
#define TURET_CANON_RELOAD   0.4f


IMPLEMENT_1LIST_MEMBER(GC_Turret, LIST_timestep);

JobManager<GC_Turret> GC_Turret::_jobManager;

GC_Turret::GC_Turret(vec2d pos, TurretState state)
  : GC_RigidBodyStatic(pos)
  , _rotator(_dir)
  , _initialDir(0)
  , _state(state)
  , _team(0)
  , _sight(TURET_SIGHT_RADIUS)
{
	SetSize(CELL_SIZE * 2, CELL_SIZE * 2);

	_jobManager.RegisterMember(this);
	_rotator.reset(0, 0, 2.0f, 5.0f, 10.0f);
}

GC_Turret::GC_Turret(FromFile)
  : GC_RigidBodyStatic(FromFile())
  , _rotator(_dir)
{
}

GC_Turret::~GC_Turret()
{
	if( TS_WAITING == _state || TS_HIDDEN == _state )
	{
		_jobManager.UnregisterMember(this);
	}
}

void GC_Turret::Serialize(World &world, SaveFile &f)
{
	GC_RigidBodyStatic::Serialize(world, f);

	_rotator.Serialize(f);

	f.Serialize(_dir);
	f.Serialize(_initialDir);
	f.Serialize(_sight);
	f.Serialize(_state);
	f.Serialize(_team);
	f.Serialize(_target);

	if( f.loading() && (TS_WAITING == _state || TS_HIDDEN == _state) )
	{
		_jobManager.RegisterMember(this);
	}
}

GC_Vehicle* GC_Turret::EnumTargets(World &world)
{
	float min_dist = _sight*_sight;
	float dist;

	GC_Vehicle *target = nullptr;
	GC_RigidBodyStatic *pObstacle = nullptr;

	FOREACH( world.GetList(LIST_vehicles), GC_Vehicle, pDamObj )
	{
		if( !pDamObj->GetOwner() ||
			(pDamObj->GetOwner()->GetTeam() && pDamObj->GetOwner()->GetTeam() == _team) )
		{
			continue;
		}

		dist = (GetPos().x - pDamObj->GetPos().x)*(GetPos().x - pDamObj->GetPos().x)
				+ (GetPos().y - pDamObj->GetPos().y)*(GetPos().y - pDamObj->GetPos().y);

		if( dist < min_dist && IsTargetVisible(world, pDamObj, &pObstacle) )
		{
			target = pDamObj;
			min_dist = dist;
		}
	}

	return target;
}

void GC_Turret::SetFire(World &world, bool fire)
{
	if (CheckFlags(GC_FLAG_TURRET_FIRE) != fire)
	{
		SetFlags(GC_FLAG_TURRET_FIRE, fire);
		for( auto ls: world.eGC_Turret._listeners )
			ls->OnFireStateChange(*this);
	}
}

void GC_Turret::SelectTarget(World &world, GC_Vehicle *target)
{
	_jobManager.UnregisterMember(this);
	_target = target;
	SetState(world, TS_ATACKING);
}

void GC_Turret::TargetLost()
{
	_jobManager.RegisterMember(this);
	_target = nullptr;
	_state  = TS_WAITING;
}

bool GC_Turret::IsTargetVisible(World &world, GC_Vehicle* target, GC_RigidBodyStatic** pObstacle)
{
	GC_RigidBodyStatic *object = world.TraceNearest(
		world.grid_rigid_s, this, GetPos(), target->GetPos() - GetPos());

	if( object != target )
	{
		*pObstacle = object;
		return false;
	}

	*pObstacle = nullptr;
	return true;
}

void GC_Turret::OnDestroy(World &world, const DamageDesc &dd)
{
	world.New<GC_ExplosionBig>(GetPos());
	GC_RigidBodyStatic::OnDestroy(world, dd);
}

void GC_Turret::ProcessState(World &world, float dt)
{
	bool fire = false;
	switch( GetState() )
	{
	case TS_WAITING:
		if( _jobManager.TakeJob(this) )
		{
			if( GC_Vehicle *target = EnumTargets(world) )
				SelectTarget(world, target);
		}
		break;
	case TS_ATACKING:
	{
		GC_RigidBodyStatic *pObstacle = nullptr;
		if( _target )
		{
			if( IsTargetVisible(world, _target, &pObstacle) )
			{
				vec2d fake;
				CalcOutstrip(world, _target, fake);

				float ang2 = ( fake - GetPos() ).Angle();

				_rotator.rotate_to(ang2);

				float d1 = fabsf(ang2-_dir);
				float d2 = _dir < ang2 ? _dir-ang2+PI2 : ang2-_dir+PI2;

				if( std::min(d1, d2) < (PI / 36.0) )
				{
					fire = true;
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
		assert(false);
	}  // switch

	SetFire(world, fire);
}

void GC_Turret::TimeStep(World &world, float dt)
{
	RotatorState prevState = _rotator.GetState();
	_rotator.process_dt(dt);

	ProcessState(world, dt);

	if (_rotator.GetState() != prevState)
	{
		for( auto ls: world.eGC_Turret._listeners )
			ls->OnRotationStateChange(*this);
	}
}

void GC_Turret::SetInitialDir(float initialDir)
{
	_dir = initialDir;
	_initialDir = _dir;
}

void GC_Turret::Kill(World &world)
{
	SetFire(world, false);
	if (_rotator.GetState() != RS_STOPPED)
	{
		_rotator.stop(true);
		for( auto ls: world.eGC_Turret._listeners )
			ls->OnRotationStateChange(*this);
	}
	GC_RigidBodyStatic::Kill(world);
}

void GC_Turret::MapExchange(MapFile &f)
{
	GC_RigidBodyStatic::MapExchange(f);

	MAP_EXCHANGE_FLOAT(sight_radius,  _sight, TURET_SIGHT_RADIUS);
	MAP_EXCHANGE_FLOAT(dir, _initialDir, 0);
	MAP_EXCHANGE_INT  (team, _team, 0);

	if( f.loading() )
	{
		if( _team > MAX_TEAMS-1 )
			_team = MAX_TEAMS-1;
		_dir = _initialDir;
	}
}

void GC_Turret::SetState(World &world, TurretState state)
{
	if (_state != state)
	{
		_state = state;
		for( auto ls: world.eGC_Turret._listeners )
			ls->OnStateChange(*this);
	}
}

void GC_Turret::Shoot(World &world)
{
	for( auto ls: world.eGC_Turret._listeners )
		ls->OnShoot(*this);
	OnShoot(world);
}

PropertySet* GC_Turret::NewPropertySet()
{
	return new MyPropertySet(this);
}

GC_Turret::MyPropertySet::MyPropertySet(GC_Object *object)
  : BASE(object)
  , _propTeam(  ObjectProperty::TYPE_INTEGER, "team"  )
  , _propSight( ObjectProperty::TYPE_INTEGER, "sight" )
  , _propDir(   ObjectProperty::TYPE_FLOAT,   "dir" )
{
	_propTeam.SetIntRange(0, MAX_TEAMS - 1);
	_propSight.SetIntRange(0, 100);
    _propDir.SetFloatRange(0, PI2);
}

int GC_Turret::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 3;
}

ObjectProperty* GC_Turret::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
		case 0: return &_propTeam;
		case 1: return &_propSight;
        case 2: return &_propDir;
	}

	assert(false);
	return nullptr;
}

void GC_Turret::MyPropertySet::MyExchange(World &world, bool applyToObject)
{
	BASE::MyExchange(world, applyToObject);

	GC_Turret *tmp = static_cast<GC_Turret *>(GetObject());

	if( applyToObject )
	{
		tmp->_team  = _propTeam.GetIntValue();
		tmp->_sight = (float) (_propSight.GetIntValue() * CELL_SIZE);
        tmp->SetInitialDir(_propDir.GetFloatValue());
	}
	else
	{
		_propTeam.SetIntValue(tmp->_team);
		_propSight.SetIntValue(int(tmp->_sight / CELL_SIZE + 0.5f));
        _propDir.SetFloatValue(tmp->_initialDir);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_TurretRocket)
{
	ED_TURRET( "turret_rocket", "obj_turret_rocket" );
	return true;
}

GC_TurretRocket::GC_TurretRocket(vec2d pos)
  : GC_Turret(pos, TS_WAITING)
  , _timeReload(0)
{
	SetHealth(GetDefaultHealth(), GetDefaultHealth());
}

GC_TurretRocket::GC_TurretRocket(FromFile)
  : GC_Turret(FromFile())
{
}

GC_TurretRocket::~GC_TurretRocket()
{
}

void GC_TurretRocket::Serialize(World &world, SaveFile &f)
{
	GC_Turret::Serialize(world, f);
	f.Serialize(_timeReload);
}

void GC_TurretRocket::CalcOutstrip(World &world, const GC_Vehicle *target, vec2d &fake)
{
	world.CalcOutstrip(GetPos(), SPEED_ROCKET, target->GetPos(), target->_lv, fake);
}

void GC_TurretRocket::TimeStep(World &world, float dt)
{
	GC_Turret::TimeStep(world, dt);
	if( GetFire() && _timeReload <= 0 )
	{
		Shoot(world);
		_timeReload = TURET_ROCKET_RELOAD;
	}
	_timeReload -= dt;
}

void GC_TurretRocket::OnShoot(World &world)
{
	vec2d a = Vec2dDirection(GetWeaponDir());
	auto &rocket = world.New<GC_Rocket>(GetPos() + a * 25.0f, a * SPEED_ROCKET, this, nullptr, true);
	rocket.SelectTarget(world);
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_TurretCannon)
{
	ED_TURRET( "turret_cannon", "obj_turret_cannon" );
	return true;
}

GC_TurretCannon::GC_TurretCannon(vec2d pos)
  : GC_Turret(pos, TS_WAITING)
  , _timeReload(0)
  , _time_smoke(0)
  , _time_smoke_dt(0)
{
	SetHealth(GetDefaultHealth(), GetDefaultHealth());
}

GC_TurretCannon::GC_TurretCannon(FromFile)
  : GC_Turret(FromFile())
{
}

GC_TurretCannon::~GC_TurretCannon()
{
}

void GC_TurretCannon::Serialize(World &world, SaveFile &f)
{
	GC_Turret::Serialize(world, f);
	f.Serialize(_timeReload);
	f.Serialize(_time_smoke);
	f.Serialize(_time_smoke_dt);
}

void GC_TurretCannon::CalcOutstrip(World &world, const GC_Vehicle *target, vec2d &fake)
{
	world.CalcOutstrip(GetPos(), SPEED_TANKBULLET, target->GetPos(), target->_lv, fake);
}

void GC_TurretCannon::TimeStep(World &world, float dt)
{
	GC_Turret::TimeStep(world, dt);

	if( GetFire() && _timeReload <= 0 )
	{
		Shoot(world);
		_timeReload = TURET_CANON_RELOAD;
		_time_smoke  = 0.1f;
	}

	_timeReload -= dt;

	if( _time_smoke > 0 )
	{
		_time_smoke -= dt;
		_time_smoke_dt += dt;
		for( ;_time_smoke_dt > 0; _time_smoke_dt -= 0.025f )
		{
			world.New<GC_Particle>(GetPos() + Vec2dDirection(GetWeaponDir()) * 33.0f,
			                       SPEED_SMOKE + Vec2dDirection(GetWeaponDir()) * 50,
			                       PARTICLE_SMOKE,
			                       frand(0.3f) + 0.2f);
		}
	}
}

void GC_TurretCannon::OnShoot(World &world)
{
	vec2d a = Vec2dDirection(GetWeaponDir());
	world.New<GC_TankBullet>(GetPos() + a * 31.9f, a * SPEED_TANKBULLET + world.net_vrand(40), this, nullptr, false);
}

////////////////////////////////////////////////////////////////////

GC_TurretBunker::GC_TurretBunker(vec2d pos)
  : GC_Turret(pos, TS_HIDDEN)
  , _time(0)
  , _time_wait_max(1.0f)
  , _time_wait(_time_wait_max)
  , _time_wake(0)  // hidden
  , _time_wake_max(0.5f)
{
	_rotator.setl(2.0f, 20.0f, 30.0f);
}

GC_TurretBunker::GC_TurretBunker(FromFile)
  : GC_Turret(FromFile())
{
}

void GC_TurretBunker::Serialize(World &world, SaveFile &f)
{
	GC_Turret::Serialize(world, f);
	f.Serialize(_delta_angle);
	f.Serialize(_time);
	f.Serialize(_time_wait);
	f.Serialize(_time_wait_max);
	f.Serialize(_time_wake);
	f.Serialize(_time_wake_max);
}

void GC_TurretBunker::MapExchange(MapFile &f)
{
	GC_Turret::MapExchange(f);
	if( f.loading() )
	{
		SetDirection(Vec2dDirection(_initialDir));
	}
}

GC_TurretBunker::~GC_TurretBunker()
{
}

void GC_TurretBunker::WakeUp(World &world)
{
	_jobManager.UnregisterMember(this);
	SetState(world, TS_WAKING_UP);
}

void GC_TurretBunker::WakeDown(World &world)
{
	SetState(world, TS_PREPARE_TO_WAKEDOWN);
	_jobManager.UnregisterMember(this);
}

void GC_TurretBunker::OnDamage(World &world, DamageDesc &dd)
{
	if( GetState() == TS_HIDDEN )
	{
		dd.damage = 0;
	}
	GC_Turret::OnDamage(world, dd);
}

void GC_TurretBunker::ProcessState(World &world, float dt)
{
	bool fire = false;

	switch( GetState() )
	{
	case TS_ATACKING:
	{
		GC_RigidBodyStatic *pObstacle = nullptr;

		if( _target )
		{
			if( IsTargetVisible(world, _target, &pObstacle) )
			{
				vec2d fake;
				CalcOutstrip(world, _target, fake);

				float ang2 = ( fake - GetPos() ).Angle();

				_rotator.rotate_to(ang2);

				float d1 = fabsf(ang2-_dir);
				float d2 = _dir < ang2 ? _dir-ang2+PI2 : ang2-_dir+PI2;

				if( std::min(d1, d2) <= _delta_angle )
                {
					fire = true;
                }
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
		_time_wait = world.net_frand(_time_wait_max);
		break;
	} // end case TS_ATACKING

	case TS_WAITING:
		_time_wait -= dt;
		if( _time_wait <= 0 )
		{
			_time_wait = world.net_frand(_time_wait_max);
			WakeDown(world);
		}
		else
		{
		//	if( _jobManager.TakeJob(this) )
			{
				if( GC_Vehicle *target = EnumTargets(world) )
					SelectTarget(world, target);
			}
		}
		break;

	case TS_HIDDEN:
		if( _jobManager.TakeJob(this) )
		{
			if( EnumTargets(world) )
                WakeUp(world);
		}
		break;

	case TS_WAKING_UP:
		_time_wake += dt;
		if( _time_wake >= _time_wake_max )
		{
			_time_wake = _time_wake_max;
			SetState(world, TS_WAITING);
			_jobManager.RegisterMember(this);
		}
		break;

	case TS_WAKING_DOWN:
		_time_wake -= dt;
		if( _time_wake <= 0 )
		{
			_time_wake = 0;
			SetState(world, TS_HIDDEN);
			_jobManager.RegisterMember(this);
		}
		break;

	case TS_PREPARE_TO_WAKEDOWN:
		if( _initialDir != _dir || RS_STOPPED != _rotator.GetState() )
		{
			_rotator.rotate_to(_initialDir);
		}
		else
		{
			SetState(world, TS_WAKING_DOWN);
		}
		break;
	default:
		assert(0);
	} // switch

	SetFire(world, fire);
}

void GC_TurretBunker::SetInitialDir(float initialDir)
{
	_initialDir = initialDir;
	_dir = _initialDir;
	SetDirection(Vec2dDirection(_initialDir));
}

////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_TurretMinigun)
{
	ED_TURRET( "turret_minigun", "obj_turret_minigun" );
	return true;
}

GC_TurretMinigun::GC_TurretMinigun(vec2d pos)
  : GC_TurretBunker(pos)
{
	_delta_angle = 0.5f; // shooting accuracy
	_rotator.reset(0, 0, 6.0f, 21.0f, 36.0f);

	_time = 0;

	_time_wait_max = 1.0f;
	_time_wait     = _time_wait_max;
	_time_wake_max = 0.5f;

	SetHealth(GetDefaultHealth(), GetDefaultHealth());
}

GC_TurretMinigun::GC_TurretMinigun(FromFile)
  : GC_TurretBunker(FromFile())
{
}

GC_TurretMinigun::~GC_TurretMinigun()
{
}

void GC_TurretMinigun::Serialize(World &world, SaveFile &f)
{
	GC_TurretBunker::Serialize(world, f);
	f.Serialize(_time);
}

void GC_TurretMinigun::CalcOutstrip(World &world, const GC_Vehicle *target, vec2d &fake)
{
	world.CalcOutstrip(GetPos(), SPEED_BULLET, target->GetPos(), target->_lv, fake);
}

void GC_TurretMinigun::TimeStep(World &world, float dt)
{
	GC_TurretBunker::TimeStep(world, dt);

	if( GetFire() )
	{
		for( _time += dt; _time > 0; _time -= 0.04f )
		{
			Shoot(world);
		}
	}
}

void GC_TurretMinigun::OnShoot(World &world)
{
	float ang = _dir + world.net_frand(0.1f) - 0.05f;
	vec2d a = Vec2dDirection(_dir);
	world.New<GC_Bullet>(GetPos() + a * 31.9f, Vec2dDirection(ang) * SPEED_BULLET, this, nullptr, false);
	world.New<GC_Particle>(GetPos() + a * 31.9f, a * (400 + frand(400.0f)), PARTICLE_TYPE1, frand(0.06f) + 0.03f);
}

////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_TurretGauss)
{
	ED_TURRET( "turret_gauss", "obj_turret_gauss" );
	return true;
}

GC_TurretGauss::GC_TurretGauss(vec2d pos)
  : GC_TurretBunker(pos)
{
	_delta_angle = 0.03f; // shooting accuracy
	_rotator.reset(0, 0, 10.0f, 30.0f, 60.0f);

	_time = 0;
	_shotCount = 0;

	_time_wait_max = 0.10f;
	_time_wake_max = 0.45f;

	SetHealth(GetDefaultHealth(), GetDefaultHealth());
}

GC_TurretGauss::GC_TurretGauss(FromFile)
  : GC_TurretBunker(FromFile())
{
}

GC_TurretGauss::~GC_TurretGauss()
{
}

void GC_TurretGauss::TargetLost()
{
	_shotCount = 0;
	_time      = 0;
	GC_TurretBunker::TargetLost();
}

void GC_TurretGauss::Serialize(World &world, SaveFile &f)
{
	GC_TurretBunker::Serialize(world, f);
	f.Serialize(_shotCount);
	f.Serialize(_time);
}

void GC_TurretGauss::CalcOutstrip(World &world, const GC_Vehicle *target, vec2d &fake)
{
	world.CalcOutstrip(GetPos(), SPEED_GAUSS, target->GetPos(), target->_lv, fake);
}

void GC_TurretGauss::TimeStep(World &world, float dt)
{
	GC_TurretBunker::TimeStep(world, dt);
	if (GetFire())
	{
		if( 0 == _shotCount )
		{
			_time = 0;
		}

		if( _time >= (float) _shotCount * 0.2f )
		{
			Shoot(world);
			if( ++_shotCount == 2 )
			{
				TargetLost();
				WakeDown(world);
			}
		}
	}
	_time += dt;
}

void GC_TurretGauss::OnShoot(World &world)
{
	float dy = _shotCount == 0 ? -7.0f : 7.0f;
	float c = cosf(_dir), s = sinf(_dir);

	world.New<GC_GaussRay>(
		vec2d{ GetPos().x + c * 20.0f - dy * s, GetPos().y + s * 20.0f + dy * c },
		vec2d{ c, s } * SPEED_GAUSS, this, nullptr, false);
}
