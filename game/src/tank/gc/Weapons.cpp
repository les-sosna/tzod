#include "Weapons.h"
#include "Vehicle.h"
#include "VehicleClasses.h"
#include "RigidBodyDinamic.h"
#include "Sound.h"
#include "Light.h"
#include "projectiles.h"
#include "particles.h"

#include "Macros.h"
#include "World.h"

#include "SaveFile.h"

#include <cfloat>


IMPLEMENT_SELF_REGISTRATION(GC_Weap_RocketLauncher)
{
	ED_ITEM("weap_rockets", "obj_weap_rockets", 4);
	return true;
}

GC_Weap_RocketLauncher::GC_Weap_RocketLauncher(World &world)
  : GC_ProjectileBasedWeapon(world)
{
}

GC_Weap_RocketLauncher::GC_Weap_RocketLauncher(FromFile)
  : GC_ProjectileBasedWeapon(FromFile())
{
}

static float AdjustHealth(float value)
{
	const float defaultVehicleClassHealth = 400;
	return value / defaultVehicleClassHealth;
}

void GC_Weap_RocketLauncher::AdjustVehicleClass(VehicleClass &vc) const
{
	vc.health *= AdjustHealth(85);
	vc.m *= 1.16f;
	vc.i *= 1.2f;
}

void GC_Weap_RocketLauncher::OnShoot(World &world)
{
	unsigned int nshots = GetAdvanced() ? world.net_rand() % SERIES_LENGTH : GetNumShots();
	float dy = (0.5f - (float) nshots / (float) (SERIES_LENGTH - 1)) * 15.0f;

	SetLastShotPos(vec2d(13, dy));
	
	vec2d dir = GetDirection();
	float ax = dir.x * 15.0f + dy * dir.y;
	float ay = dir.y * 15.0f - dy * dir.x;
	
	(new GC_Rocket(world, GetCarrier()->GetPos() + vec2d(ax, ay),
				   Vec2dAddDirection(dir, vec2d(world.net_frand(0.1f) - 0.05f)) * SPEED_ROCKET,
				   GetCarrier(), GetCarrier()->GetOwner(), GetAdvanced()))->Register(world);
}

void GC_Weap_RocketLauncher::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = true;
	pSettings->fMaxAttackAngleCos = cos(0.2f);
	pSettings->fProjectileSpeed   = SPEED_ROCKET;
	pSettings->fAttackRadius_max  = 600.0f;
	pSettings->fAttackRadius_min  = 100.0f;
	pSettings->fAttackRadius_crit =  40.0f;
	pSettings->fDistanceMultipler = GetAdvanced() ? 1.2f : 3.5f;
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_AutoCannon)
{
	ED_ITEM( "weap_autocannon", "obj_weap_autocannon", 4 );
	return true;
}

GC_Weap_AutoCannon::GC_Weap_AutoCannon(World &world)
  : GC_ProjectileBasedWeapon(world)
{
}

GC_Weap_AutoCannon::GC_Weap_AutoCannon(FromFile)
  : GC_ProjectileBasedWeapon(FromFile())
{
}

void GC_Weap_AutoCannon::AdjustVehicleClass(VehicleClass &vc) const
{
	vc.health *= AdjustHealth(80);
}

void GC_Weap_AutoCannon::OnShoot(World &world)
{
	vec2d dir = GetDirection();
	
	if (GetAdvanced())
	{
		SetLastShotPos(vec2d(17.0f, 0));
		for( int t = 0; t < 2; ++t )
		{
			float dy = t == 0 ? -9.0f : 9.0f;
			
			float ax = dir.x * 17.0f - dy * dir.y;
			float ay = dir.y * 17.0f + dy * dir.x;
			
			(new GC_ACBullet(world, GetCarrier()->GetPos() + vec2d(ax, ay),
							 dir * SPEED_ACBULLET,
							 GetCarrier(), GetCarrier()->GetOwner(), GetAdvanced()))->Register(world);
		}
	}
	else
	{
		float dy = (GetNumShots() % 2) == 0 ? -9.0f : 9.0f;
		SetLastShotPos(vec2d(17.0f, -dy));
		
		float ax = dir.x * 17.0f - dy * dir.y;
		float ay = dir.y * 17.0f + dy * dir.x;
		
		(new GC_ACBullet(world, GetCarrier()->GetPos() + vec2d(ax, ay),
						 Vec2dAddDirection(dir, vec2d(world.net_frand(0.02f) - 0.01f)) * SPEED_ACBULLET,
						 GetCarrier(), GetCarrier()->GetOwner(), GetAdvanced()))->Register(world);
	}
}

void GC_Weap_AutoCannon::SetAdvanced(World &world, bool advanced)
{
	ResetSeries();
	GC_ProjectileBasedWeapon::SetAdvanced(world, advanced);
}

void GC_Weap_AutoCannon::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = true;
	pSettings->fMaxAttackAngleCos = cos(0.1f);
	pSettings->fProjectileSpeed   = SPEED_ACBULLET;
	pSettings->fAttackRadius_max  = 500;
	pSettings->fAttackRadius_min  = 100;
	pSettings->fAttackRadius_crit =   0;
	pSettings->fDistanceMultipler = GetAdvanced() ? 3.3f : 13.0f;
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Cannon)
{
	ED_ITEM( "weap_cannon", "obj_weap_cannon", 4 );
	return true;
}

GC_Weap_Cannon::GC_Weap_Cannon(World &world)
  : GC_ProjectileBasedWeapon(world)
{
	SetLastShotPos(vec2d(21, 0));
}

void GC_Weap_Cannon::Attach(World &world, GC_Actor *actor)
{
	GC_ProjectileBasedWeapon::Attach(world, actor);

	_time_smoke_dt = 0;
	_time_smoke    = 0;
}

GC_Weap_Cannon::GC_Weap_Cannon(FromFile)
  : GC_ProjectileBasedWeapon(FromFile())
{
}

void GC_Weap_Cannon::Serialize(World &world, SaveFile &f)
{
	GC_ProjectileBasedWeapon::Serialize(world, f);
	f.Serialize(_time_smoke);
	f.Serialize(_time_smoke_dt);
}

void GC_Weap_Cannon::AdjustVehicleClass(VehicleClass &vc) const
{
	vc.health *= AdjustHealth(125);
}

void GC_Weap_Cannon::OnShoot(World &world)
{
	_time_smoke = 0.3f;

	GC_Vehicle * const veh = static_cast<GC_Vehicle*>(GetCarrier());
	const vec2d &dir = GetDirection();
	
	(new GC_TankBullet(world, GetPos() + dir * 17.0f,
					   dir * SPEED_TANKBULLET + world.net_vrand(50), veh, veh->GetOwner(), GetAdvanced()))->Register(world);
	
	if( !GetAdvanced() )
	{
		veh->ApplyImpulse( dir * (-80.0f) );
	}
}

void GC_Weap_Cannon::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = true;
	pSettings->fMaxAttackAngleCos = cos(0.1f);
	pSettings->fProjectileSpeed   = SPEED_TANKBULLET;
	pSettings->fAttackRadius_max  = 500;
	pSettings->fAttackRadius_min  = 100;
	pSettings->fAttackRadius_crit = GetAdvanced() ? 64.0f : 0;
	pSettings->fDistanceMultipler = GetAdvanced() ? 2.0f : 8.0f;
}

void GC_Weap_Cannon::TimeStep(World &world, float dt)
{
	GC_ProjectileBasedWeapon::TimeStep(world, dt);

	if( GetCarrier() && _time_smoke > 0 )
	{
		_time_smoke -= dt;
		_time_smoke_dt += dt;

		for( ;_time_smoke_dt > 0; _time_smoke_dt -= 0.025f )
		{
			auto p = new GC_Particle(world, SPEED_SMOKE + GetDirection() * 50.0f, PARTICLE_SMOKE, frand(0.3f) + 0.2f);
            p->Register(world);
            p->MoveTo(world, GetPos() + GetDirection() * 26.0f);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Plazma)
{
	ED_ITEM( "weap_plazma", "obj_weap_plazma", 4 );
	return true;
}

GC_Weap_Plazma::GC_Weap_Plazma(World &world)
  : GC_ProjectileBasedWeapon(world)
{
}

GC_Weap_Plazma::GC_Weap_Plazma(FromFile)
  : GC_ProjectileBasedWeapon(FromFile())
{
}

void GC_Weap_Plazma::AdjustVehicleClass(VehicleClass &vc) const
{
	vc.health *= AdjustHealth(100);
}

void GC_Weap_Plazma::OnShoot(World &world)
{
	const vec2d &a = GetDirection();
	(new GC_PlazmaClod(world, GetPos() + a * 15.0f,
					   a * SPEED_PLAZMA + world.net_vrand(20),
					   GetCarrier(), GetCarrier()->GetOwner(), GetAdvanced()))->Register(world);
}

void GC_Weap_Plazma::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = true;
	pSettings->fMaxAttackAngleCos = cos(0.2f);
	pSettings->fProjectileSpeed   = SPEED_PLAZMA;
	pSettings->fAttackRadius_max  = 300;
	pSettings->fAttackRadius_min  = 100;
	pSettings->fAttackRadius_crit = 0;
	pSettings->fDistanceMultipler = GetAdvanced() ? 2.0f : 8.0f;  // fixme
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Gauss)
{
	ED_ITEM( "weap_gauss", "obj_weap_gauss", 4 );
	return true;
}

GC_Weap_Gauss::GC_Weap_Gauss(World &world)
  : GC_ProjectileBasedWeapon(world)
{
}

GC_Weap_Gauss::GC_Weap_Gauss(FromFile)
  : GC_ProjectileBasedWeapon(FromFile())
{
}

void GC_Weap_Gauss::AdjustVehicleClass(VehicleClass &vc) const
{
	vc.health *= AdjustHealth(70);
}

void GC_Weap_Gauss::OnShoot(World &world)
{
	const vec2d &dir = GetDirection();
	(new GC_GaussRay(world, vec2d(GetPos().x + dir.x + 5 * dir.y, GetPos().y + dir.y - 5 * dir.x),
					 dir * SPEED_GAUSS, GetCarrier(), GetCarrier()->GetOwner(), GetAdvanced()))->Register(world);
}

void GC_Weap_Gauss::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = false;
	pSettings->fMaxAttackAngleCos = cos(0.01f);
	pSettings->fProjectileSpeed   = 0;
	pSettings->fAttackRadius_max  = 800;
	pSettings->fAttackRadius_min  = 400;
	pSettings->fAttackRadius_crit = 0;
	pSettings->fDistanceMultipler = GetAdvanced() ? 4.5f : 9.0f;
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Ram)
{
	ED_ITEM( "weap_ram", "obj_weap_ram", 4 );
	return true;
}

GC_Weap_Ram::GC_Weap_Ram(World &world)
  : GC_Weapon(world)
  , _firingCounter(0)
{
}

void GC_Weap_Ram::SetAdvanced(World &world, bool advanced)
{
	if( GetCarrier() )
	{
		static_cast<GC_Vehicle*>(GetCarrier())->_percussion =
			advanced ? WEAP_RAM_PERCUSSION * 2 : WEAP_RAM_PERCUSSION;
	}

	GC_Weapon::SetAdvanced(world, advanced);
}

void GC_Weap_Ram::Attach(World &world, GC_Actor *actor)
{
	GC_Weapon::Attach(world, actor);

	_engineSound = new GC_Sound(world, SND_RamEngine, GetPos());
    _engineSound->Register(world);
    _engineSound->SetMode(world, SMODE_STOP);
	_engineLight = new GC_Light(world, GC_Light::LIGHT_POINT);
    _engineLight->Register(world);
	_engineLight->SetIntensity(1.0f);
	_engineLight->SetRadius(120);
	_engineLight->SetActive(false);


	_fuel_max  = _fuel = 1.0f;
	_fuel_consumption_rate = 0.2f;
	_fuel_recuperation_rate  = 0.1f;

	_firingCounter = 0;
	_bReady = true;
}

void GC_Weap_Ram::Detach(World &world)
{
	SAFE_KILL(world, _engineSound);
	SAFE_KILL(world, _engineLight);

	GC_Weapon::Detach(world);
}

GC_Weap_Ram::GC_Weap_Ram(FromFile)
  : GC_Weapon(FromFile())
{
}

GC_Weap_Ram::~GC_Weap_Ram()
{
}

void GC_Weap_Ram::Kill(World &world)
{
	SAFE_KILL(world, _engineSound);
    GC_Weapon::Kill(world);
}

void GC_Weap_Ram::OnUpdateView(World &world)
{
	_engineLight->MoveTo(world, GetPos() - GetDirection() * 20);
}

void GC_Weap_Ram::Serialize(World &world, SaveFile &f)
{
	GC_Weapon::Serialize(world, f);
	f.Serialize(_firingCounter);
	f.Serialize(_bReady);
	f.Serialize(_fuel);
	f.Serialize(_fuel_max);
	f.Serialize(_fuel_consumption_rate);
	f.Serialize(_fuel_recuperation_rate);
	f.Serialize(_engineSound);
	f.Serialize(_engineLight);
}

void GC_Weap_Ram::AdjustVehicleClass(VehicleClass &vc) const
{
	vc.health *= AdjustHealth(350);
	vc.m *= 2.0f;
	vc.enginePower *= 2.0f;
	vc.percussion *= 5.0f;
	vc.fragility *= 0.5f;
}

void GC_Weap_Ram::Fire(World &world, bool fire)
{
	assert(GetCarrier());
	if( fire && _bReady )
	{
		_firingCounter = 2;
		if( GC_RigidBodyDynamic *owner = dynamic_cast<GC_RigidBodyDynamic *>(GetCarrier()) )
		{
			owner->ApplyForce(GetDirection() * 2000);
		}
	}
}

void GC_Weap_Ram::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = false;
	pSettings->fMaxAttackAngleCos = cos(0.3f);
	pSettings->fProjectileSpeed   = 0;
	pSettings->fAttackRadius_max  = 100;
	pSettings->fAttackRadius_min  = 0;
	pSettings->fAttackRadius_crit = 0;
	pSettings->fDistanceMultipler = GetAdvanced() ? 2.5f : 6.0f;
}

void GC_Weap_Ram::TimeStep(World &world, float dt)
{
	GC_Weapon::TimeStep(world, dt);

	if( GetCarrier() && _firingCounter )
	{
		GC_Vehicle *veh = static_cast<GC_Vehicle *>(GetCarrier());
		vec2d v = veh->_lv;
		
		// primary
		{
			const vec2d &a = GetDirection();
			vec2d emitter = GetPos() - a * 20.0f;
			for( int i = 0; i < 29; ++i )
			{
				float time = frand(0.05f) + 0.02f;
				float t = frand(6.0f) - 3.0f;
				vec2d dx(-a.y * t, a.x * t);
				auto p = new GC_Particle(world, v - a * frand(800.0f) - dx / time, fabs(t) > 1.5 ? PARTICLE_FIRE2 : PARTICLE_YELLOW, time);
				p->Register(world);
				p->MoveTo(world, emitter + dx);
			}
		}
		
		
		// secondary
		for( float l = -1; l < 2; l += 2 )
		{
			vec2d a = Vec2dAddDirection(GetDirection(), vec2d(l * 0.15f));
			vec2d emitter = GetPos() - a * 15.0f + vec2d( -a.y, a.x) * l * 17.0f;
			for( int i = 0; i < 10; i++ )
			{
				float time = frand(0.05f) + 0.02f;
				float t = frand(2.5f) - 1.25f;
				vec2d dx(-a.y * t, a.x * t);
				auto p = new GC_Particle(world, v - a * frand(600.0f) - dx / time, PARTICLE_FIRE1, time);
				p->Register(world);
				p->MoveTo(world, emitter + dx);
			}
		}
	}
	
	if( GetCarrier() )
	{
		assert(_engineSound);

		if( GetAdvanced() )
			_fuel = _fuel_max;

		if( _firingCounter )
		{
			_engineSound->Pause(world, false);
			_engineSound->MoveTo(world, GetPos());

			_fuel = std::max(.0f, _fuel - _fuel_consumption_rate * dt);
			if( 0 == _fuel ) _bReady = false;

			DamageDesc dd;
			dd.from = GetCarrier()->GetOwner();

			// the primary jet
			{
				const float lenght = 50.0f;
				const vec2d &a = GetDirection();
				vec2d emitter = GetPos() - a * 20.0f;
				if( GC_RigidBodyStatic *object = world.TraceNearest(world.grid_rigid_s, GetCarrier(), emitter, -a * lenght, &dd.hit) )
				{
					dd.damage = dt * DAMAGE_RAM_ENGINE * (1.0f - (dd.hit - emitter).len() / lenght);
					object->TakeDamage(world, dd);
				}
			}

			// secondary jets
			for( float l = -1; l < 2; l += 2 )
			{
				const float lenght = 50.0f;
				vec2d a = Vec2dAddDirection(GetDirection(), vec2d(l * 0.15f));
				vec2d emitter = GetPos() - a * 15.0f + vec2d( -a.y, a.x) * l * 17.0f;
				if( GC_RigidBodyStatic *object = world.TraceNearest(world.grid_rigid_s, GetCarrier(), emitter + a * 2.0f, -a * lenght, &dd.hit) )
				{
					dd.damage = dt * DAMAGE_RAM_ENGINE * (1.0f - (dd.hit - emitter).len() / lenght);
					object->TakeDamage(world, dd);
				}
			}
		}
		else
		{
			_engineSound->Pause(world, true);
			_fuel   = std::min(_fuel_max, _fuel + _fuel_recuperation_rate * dt);
			_bReady = (_fuel_max < _fuel * 4.0f);
		}

		_engineLight->SetActive(_firingCounter > 0);
		if( _firingCounter ) --_firingCounter;
	}
	else
	{
		assert(!_engineSound);
	}

	return;
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_BFG)
{
	ED_ITEM( "weap_bfg", "obj_weap_bfg", 4 );
	return true;
}

GC_Weap_BFG::GC_Weap_BFG(World &world)
  : GC_ProjectileBasedWeapon(world)
{
}

GC_Weap_BFG::GC_Weap_BFG(FromFile)
  : GC_ProjectileBasedWeapon(FromFile())
{
}

void GC_Weap_BFG::AdjustVehicleClass(VehicleClass &vc) const
{
	vc.health *= AdjustHealth(110);
}

void GC_Weap_BFG::OnShoot(World &world)
{
	if (GetNumShots() || GetAdvanced())
	{
		(new GC_BfgCore(world, GetPos() + GetDirection() * 16.0f, GetDirection() * SPEED_BFGCORE,
						GetCarrier(), GetCarrier()->GetOwner(), GetAdvanced()))->Register(world);
	}
}

void GC_Weap_BFG::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = true;
	pSettings->fMaxAttackAngleCos = cos(0.01f);
	pSettings->fProjectileSpeed   = SPEED_BFGCORE;
	pSettings->fAttackRadius_max  = 600;
	pSettings->fAttackRadius_min  = 200;
	pSettings->fAttackRadius_crit =   0;
	pSettings->fDistanceMultipler = GetAdvanced() ? 13.0f : 20.0f;
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Ripper)
{
	ED_ITEM( "weap_ripper", "obj_weap_ripper", 4 );
	return true;
}

GC_Weap_Ripper::GC_Weap_Ripper(World &world)
  : GC_ProjectileBasedWeapon(world)
{
}

GC_Weap_Ripper::GC_Weap_Ripper(FromFile)
  : GC_ProjectileBasedWeapon(FromFile())
{
}

void GC_Weap_Ripper::AdjustVehicleClass(VehicleClass &vc) const
{
	vc.health *= AdjustHealth(80);
}

void GC_Weap_Ripper::OnShoot(World &world)
{
	const vec2d &a = GetDirection();
	(new GC_Disk(world, GetPos() - a * 9.0f, a * SPEED_DISK + world.net_vrand(10),
				 GetCarrier(), GetCarrier()->GetOwner(), GetAdvanced()))->Register(world);
}

void GC_Weap_Ripper::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = true;
	pSettings->fMaxAttackAngleCos = cos(0.2f);
	pSettings->fProjectileSpeed   = SPEED_DISK;
	pSettings->fAttackRadius_max  = 700;
	pSettings->fAttackRadius_min  = 500;
	pSettings->fAttackRadius_crit =  60;
	pSettings->fDistanceMultipler = GetAdvanced() ? 2.2f : 40.0f;
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Minigun)
{
	ED_ITEM( "weap_minigun", "obj_weap_minigun", 4 );
	return true;
}

GC_Weap_Minigun::GC_Weap_Minigun(World &world)
  : GC_ProjectileBasedWeapon(world)
{
	SetLastShotPos(vec2d(20, 0));
}

GC_Weap_Minigun::GC_Weap_Minigun(FromFile)
  : GC_ProjectileBasedWeapon(FromFile())
{
}

GC_Weap_Minigun::~GC_Weap_Minigun()
{
}

float GC_Weap_Minigun::GetHeat(const World &world) const
{
	return std::max(_heat + GetLastShotTime() - world.GetTime(), 0.0f);
}

void GC_Weap_Minigun::Kill(World &world)
{
	SAFE_KILL(world, _sound);
    GC_ProjectileBasedWeapon::Kill(world);
}

void GC_Weap_Minigun::Attach(World &world, GC_Actor *actor)
{
	GC_ProjectileBasedWeapon::Attach(world, actor);
	_sound = new GC_Sound(world, SND_MinigunFire, GetPos());
    _sound->Register(world);
    _sound->SetMode(world, SMODE_STOP);
}

void GC_Weap_Minigun::Detach(World &world)
{
	_heat = 0;
	SAFE_KILL(world, _sound);
	GC_ProjectileBasedWeapon::Detach(world);
}

void GC_Weap_Minigun::Serialize(World &world, SaveFile &f)
{
	GC_ProjectileBasedWeapon::Serialize(world, f);
	f.Serialize(_heat);
	f.Serialize(_sound);
}

void GC_Weap_Minigun::AdjustVehicleClass(VehicleClass &vc) const
{
	vc.health *= AdjustHealth(65);
	vc.m *= 0.7f;
}

void GC_Weap_Minigun::OnShoot(World &world)
{
	_heat = std::min(GetHeat(world) + GetReloadTime() * 3, WEAP_MG_TIME_RELAX);

	float da = _heat * 0.07f / WEAP_MG_TIME_RELAX;
	
	vec2d a = Vec2dAddDirection(GetDirection(), vec2d(world.net_frand(da * 2.0f) - da));
	a *= (1 - world.net_frand(0.2f));
	
	GC_RigidBodyDynamic *veh = dynamic_cast<GC_RigidBodyDynamic *>(GetCarrier());
	if( veh && !GetAdvanced() )
	{
		if( world.net_frand(WEAP_MG_TIME_RELAX * 5.0f) < _heat - WEAP_MG_TIME_RELAX * 0.2f )
		{
			float m = 3000;//veh->_inv_i; // FIXME
			veh->ApplyTorque(m * (world.net_frand(1.0f) - 0.5f));
		}
	}
	
	GC_Bullet *tmp = new GC_Bullet(world, GetPos() + a * 18.0f, a * SPEED_BULLET, GetCarrier(), GetCarrier()->GetOwner(), GetAdvanced());
	tmp->Register(world);
}

void GC_Weap_Minigun::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = true;
	pSettings->fMaxAttackAngleCos = cos(0.3f);
	pSettings->fProjectileSpeed   = SPEED_BULLET;
	pSettings->fAttackRadius_max  = 200;
	pSettings->fAttackRadius_min  = 100;
	pSettings->fAttackRadius_crit =   0;
	pSettings->fDistanceMultipler = GetAdvanced() ? 5.0f : 10.0f;
}

void GC_Weap_Minigun::TimeStep(World &world, float dt)
{
	if( GetCarrier() )
	{
		if( GetFire() )
		{
			_sound->MoveTo(world, GetPos());
			_sound->Pause(world, false);
		}
		else
		{
			_sound->Pause(world, true);
		}
	}

	GC_ProjectileBasedWeapon::TimeStep(world, dt);
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Weap_Zippo)
{
	ED_ITEM( "weap_zippo", "obj_weap_zippo", 4 );
	return true;
}

GC_Weap_Zippo::GC_Weap_Zippo(World &world)
  : GC_ProjectileBasedWeapon(world)
  , _timeBurn(0)
{
}

GC_Weap_Zippo::GC_Weap_Zippo(FromFile)
  : GC_ProjectileBasedWeapon(FromFile())
{
}

GC_Weap_Zippo::~GC_Weap_Zippo()
{
}

void GC_Weap_Zippo::Kill(World &world)
{
	SAFE_KILL(world, _sound);
    GC_ProjectileBasedWeapon::Kill(world);
}

void GC_Weap_Zippo::Attach(World &world, GC_Actor *actor)
{
	GC_ProjectileBasedWeapon::Attach(world, actor);

	_sound = new GC_Sound(world, SND_RamEngine, GetPos());
    _sound->Register(world);
    _sound->SetMode(world, SMODE_STOP);
}

void GC_Weap_Zippo::Detach(World &world)
{
	_heat = 0;
	SAFE_KILL(world, _sound);
	GC_ProjectileBasedWeapon::Detach(world);
}

void GC_Weap_Zippo::Serialize(World &world, SaveFile &f)
{
	GC_ProjectileBasedWeapon::Serialize(world, f);
	f.Serialize(_heat);
	f.Serialize(_timeBurn);
	f.Serialize(_sound);
}

void GC_Weap_Zippo::AdjustVehicleClass(VehicleClass &vc) const
{
	vc.health *= AdjustHealth(130);
}

void GC_Weap_Zippo::OnShoot(World &world)
{
	_heat = std::max(_heat - world.GetTime() + GetLastShotTime(), 0.0f);
	_heat = std::min(_heat + GetReloadTime() * 2, WEAP_ZIPPO_TIME_RELAX);
	
	GC_RigidBodyDynamic *veh = dynamic_cast<GC_RigidBodyDynamic *>(GetCarrier());
	
	vec2d vvel = veh ? veh->_lv : vec2d(0,0);
	
	vec2d a(GetDirection());
	a *= (1 - world.net_frand(0.2f));
	
	GC_FireSpark *tmp = new GC_FireSpark(world, GetPos() + a * 18.0f,
										 vvel + a * SPEED_FIRE, GetCarrier(), GetCarrier()->GetOwner(), GetAdvanced());
	tmp->Register(world);
	tmp->SetLifeTime(_heat);
	tmp->SetHealOwner(GetAdvanced());
	tmp->SetSetFire(true);
}

void GC_Weap_Zippo::SetupAI(AIWEAPSETTINGS *pSettings)
{
	pSettings->bNeedOutstrip      = true;
	pSettings->fMaxAttackAngleCos = cos(0.5f);
	pSettings->fProjectileSpeed   = SPEED_FIRE;
	pSettings->fAttackRadius_max  = 300;
	pSettings->fAttackRadius_min  = 100;
	pSettings->fAttackRadius_crit =  10;
	pSettings->fDistanceMultipler = GetAdvanced() ? 5.0f : 10.0f;
}

void GC_Weap_Zippo::TimeStep(World &world, float dt)
{
	if( GetCarrier() )
	{
		if( GetFire() )
		{
			_sound->MoveTo(world, GetPos());
			_sound->Pause(world, false);
		}
		else
		{
			_sound->Pause(world, true);
		}
	}

	if( GetAdvanced() )
	{
		_timeBurn += dt;
		while( _timeBurn > 0 )
		{
			GC_FireSpark *tmp = new GC_FireSpark(world, GetPos() + world.net_vrand(33),
				SPEED_SMOKE/2, GetCarrier(), GetCarrier() ? GetCarrier()->GetOwner() : NULL, true);
            tmp->Register(world);
			tmp->SetLifeTime(0.3f);
			tmp->TimeStep(world, _timeBurn);
			_timeBurn -= 0.01f;
		}
	}

	GC_ProjectileBasedWeapon::TimeStep(world, dt);
}
