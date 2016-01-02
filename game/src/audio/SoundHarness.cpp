#include "SoundHarness.h"
#include "SoundTemplates.h"
#include "inc/audio/SoundRender.h"
#include <gc/Crate.h>
#include <gc/Explosion.h>
#include <gc/Pickup.h>
#include <gc/Projectiles.h>
#include <gc/Turrets.h>
#include <gc/Vehicle.h>
#include <gc/Wall.h>
#include <gc/Weapons.h>
#include <gc/WeapCfg.h>
#include <gc/World.h>


SoundHarness::SoundHarness(SoundRender &soundRender, World &world)
	: _world(world)
	, _soundRender(soundRender)
{
	_world.eGC_Pickup.AddListener(*this);
	_world.eGC_Projectile.AddListener(*this);
	_world.eGC_ProjectileBasedWeapon.AddListener(*this);
	_world.eGC_pu_Shield.AddListener(*this);
	_world.eGC_RigidBodyStatic.AddListener(*this);
	_world.eGC_RigidBodyDynamic.AddListener(*this);
	_world.eGC_Turret.AddListener(*this);
	_world.eGC_Vehicle.AddListener(*this);
	_world.eWorld.AddListener(*this);
}

SoundHarness::~SoundHarness()
{
	_world.eWorld.RemoveListener(*this);
	_world.eGC_Vehicle.RemoveListener(*this);
	_world.eGC_Turret.RemoveListener(*this);
	_world.eGC_RigidBodyDynamic.RemoveListener(*this);
	_world.eGC_RigidBodyStatic.RemoveListener(*this);
	_world.eGC_pu_Shield.RemoveListener(*this);
	_world.eGC_ProjectileBasedWeapon.RemoveListener(*this);
	_world.eGC_Projectile.RemoveListener(*this);
	_world.eGC_Pickup.RemoveListener(*this);
}

static void SetupVehicleMoveSound(const GC_Vehicle &vehicle, Sound &sound)
{
	float v = vehicle._lv.len() / vehicle.GetMaxSpeed();
	sound.SetPitch(std::min(1.0f, 0.5f + 0.5f * v));
	sound.SetVolume(std::min(1.0f, 0.1f + 0.9f * v));
	sound.SetPos(vehicle.GetPos());
}

void SoundHarness::SetListenerPos(vec2d pos)
{
    _soundRender.SetListenerPos(pos);
}

void SoundHarness::Step()
{
	for (auto &p: _attached)
		p.second->SetPos(p.first->GetPos());
	
	for (auto &p: _vehicleMove)
		SetupVehicleMoveSound(*p.first, *p.second);
	
	for (auto &weapSound: _weaponRotate)
	{
		const GC_Weapon &weapon = *weapSound.first;
		Sound &sound = *weapSound.second;
		if (weapon.GetRotationState() != RS_STOPPED)
		{
			float absRate = fabsf(weapon.GetRotationRate());
			sound.SetVolume(absRate);
			sound.SetPitch(0.5f + 0.5f * absRate);
			sound.SetPos(weapon.GetPos());
			sound.SetPlaying(true);
		}
		else
		{
			sound.SetPlaying(false);
		}
	}

	for (auto &fireSound: _weaponFire)
	{
		const GC_Weapon &weapon = *fireSound.first;
		Sound &sound = *fireSound.second;
		if (weapon.GetFire())
		{
			sound.SetPos(weapon.GetPos());
			sound.SetPlaying(true);
		}
		else
		{
			sound.SetPlaying(false);
		}
	}

	for (auto &p: _turretFire)
	{
		const GC_Turret &turret = *p.first;
		Sound &sound = *p.second;
		
		if (turret.GetFire())
		{
			sound.SetPos(turret.GetPos());
			sound.SetPlaying(true);
		}
		else
		{
			sound.SetPlaying(false);
		}
	}
	for (auto &p: _turretRotate)
	{
		const GC_Turret &turret = *p.first;
		Sound &sound = *p.second;
		assert(turret.GetRotationState() != RS_STOPPED);
		
		float absRate = fabsf(turret.GetRotationRate());
		sound.SetVolume(absRate);
		sound.SetPitch(0.5f + 0.5f * absRate);
		sound.SetPos(turret.GetPos());
		sound.SetPlaying(true);
	}
}

template <class F>
static std::unique_ptr<Sound> CreatePlayingLooped(SoundRender &sr, SoundTemplate st, F &&setup)
{
	std::unique_ptr<Sound> sound = sr.CreateLopped(st);
	setup(*sound);
	sound->SetPlaying(true);
	return sound;
}

void SoundHarness::OnAttach(GC_Pickup &obj, GC_Vehicle &vehicle)
{
	ObjectType type = obj.GetType();
	static std::unordered_map<ObjectType, SoundTemplate> sounds = {
		{GC_pu_Health::GetTypeStatic(), SoundTemplate::Pickup},
		{GC_pu_Shield::GetTypeStatic(), SoundTemplate::Inv},
		{GC_pu_Shock::GetTypeStatic(), SoundTemplate::ShockActivate},
	};
	auto found = sounds.find(type);
	if (sounds.end() != found)
	{
		_soundRender.PlayOnce(found->second, obj.GetPos());
	}
	else if (GC_pu_Booster::GetTypeStatic() == type)
	{
		if (vehicle.GetWeapon())
		{
			_attached.emplace(&obj, CreatePlayingLooped(_soundRender, SoundTemplate::B_Loop, [&](Sound &s){ s.SetPos(obj.GetPos()); }));
			_soundRender.PlayOnce(SoundTemplate::B_Start, obj.GetPos());
		}
	}
	else if (auto weapon = dynamic_cast<GC_Weapon*>(&obj))
	{
		_soundRender.PlayOnce(SoundTemplate::w_Pickup, obj.GetPos());
		_weaponRotate.emplace(weapon, _soundRender.CreateLopped(SoundTemplate::TowerRotate));
		if (GC_Weap_Minigun::GetTypeStatic() == type)
			_weaponFire.emplace(weapon, _soundRender.CreateLopped(SoundTemplate::MinigunFire));
		else if (GC_Weap_Zippo::GetTypeStatic() == type)
			_weaponFire.emplace(weapon, _soundRender.CreateLopped(SoundTemplate::RamEngine));
		else if (GC_Weap_Ram::GetTypeStatic() == type)
			_weaponFire.emplace(weapon, _soundRender.CreateLopped(SoundTemplate::RamEngine));
	}
}

void SoundHarness::OnDetach(GC_Pickup &obj)
{
	if (auto weapon = dynamic_cast<GC_Weapon*>(&obj))
	{
		assert(_weaponRotate.count(weapon));
		_weaponRotate.erase(weapon);
		_weaponFire.erase(weapon); // not all weapons have this
	}
	_attached.erase(&obj);
}

void SoundHarness::OnRespawn(GC_Pickup &obj)
{
	_soundRender.PlayOnce(SoundTemplate::puRespawn, obj.GetPos());
}

void SoundHarness::OnDisappear(GC_Pickup &obj)
{
	if (GC_pu_Booster::GetTypeStatic() == obj.GetType())
	{
		_soundRender.PlayOnce(SoundTemplate::B_End, obj.GetPos());
	}
}

void SoundHarness::OnHit(GC_Projectile &obj, GC_RigidBodyStatic &target, vec2d hit)
{
	static std::unordered_map<ObjectType, SoundTemplate> sounds = {
		{GC_TankBullet::GetTypeStatic(), SoundTemplate::BoomBullet},
		{GC_PlazmaClod::GetTypeStatic(), SoundTemplate::PlazmaHit},
		{GC_BfgCore::GetTypeStatic(), SoundTemplate::BfgFlash},
	};
	ObjectType type = obj.GetType();
	auto found = sounds.find(type);
	if (sounds.end() != found)
	{
		_soundRender.PlayOnce(found->second, hit);
	}
	else if (GC_ACBullet::GetTypeStatic() == type)
	{
		if( dynamic_cast<GC_Wall_Concrete *>(&target) )
			_soundRender.PlayOnce((rand() % 2) ? SoundTemplate::AC_Hit2 : SoundTemplate::AC_Hit3, hit);
		else
			_soundRender.PlayOnce(SoundTemplate::AC_Hit1, hit);
	}
	else if (GC_Disk::GetTypeStatic() == type)
	{
		if (static_cast<const GC_Disk&>(obj).GetBounces())
			_soundRender.PlayOnce(SoundTemplate::DiskHit, hit);
		else
			_soundRender.PlayOnce(SoundTemplate::BoomBullet, hit);
	}
}

void SoundHarness::OnShoot(GC_ProjectileBasedWeapon &obj)
{
	static std::unordered_map<ObjectType, SoundTemplate> sounds = {
		{GC_Weap_AutoCannon::GetTypeStatic(), SoundTemplate::ACShoot},
		{GC_Weap_Cannon::GetTypeStatic(), SoundTemplate::Shoot},
		{GC_Weap_Gauss::GetTypeStatic(), SoundTemplate::Bolt},
		{GC_Weap_Plazma::GetTypeStatic(), SoundTemplate::PlazmaFire},
		{GC_Weap_Ripper::GetTypeStatic(), SoundTemplate::DiskFire},
		{GC_Weap_RocketLauncher::GetTypeStatic(), SoundTemplate::RocketShoot},
	};
	auto found = sounds.find(obj.GetType());
	if (sounds.end() != found)
	{
		_soundRender.PlayOnce(found->second, obj.GetPos());
	}
	else if (GC_Weap_BFG::GetTypeStatic() == obj.GetType())
	{
		if( obj.GetNumShots() )
			_soundRender.PlayOnce(SoundTemplate::BfgFire, obj.GetPos());
		else if( !obj.GetBooster() )
			_soundRender.PlayOnce(SoundTemplate::BfgInit, obj.GetPos());
	}
}

void SoundHarness::OnOwnerDamage(GC_pu_Shield &obj, DamageDesc &dd)
{
	_soundRender.PlayOnce(rand() % 2 ? SoundTemplate::InvHit1 : SoundTemplate::InvHit2, obj.GetPos());
}

void SoundHarness::OnExpiring(GC_pu_Shield &obj)
{
	_soundRender.PlayOnce(SoundTemplate::InvEnd, obj.GetPos());
}

void SoundHarness::OnDestroy(GC_RigidBodyStatic &obj, const DamageDesc &dd)
{
	ObjectType type = obj.GetType();
	if (GC_Crate::GetTypeStatic() == type)
		_soundRender.PlayOnce(SoundTemplate::WallDestroy, obj.GetPos());
	else if (GC_Wall::GetTypeStatic() == type)
		_soundRender.PlayOnce(SoundTemplate::WallDestroy, obj.GetPos());
}

void SoundHarness::OnDamage(GC_RigidBodyStatic &obj, const DamageDesc &dd)
{
	if (dd.damage >= DAMAGE_BULLET)
	{
		auto turret = dynamic_cast<GC_TurretBunker*>(&obj);
		if (GC_Wall_Concrete::GetTypeStatic() == obj.GetType() ||
			(turret && turret->GetState() == TS_HIDDEN))
		{
			if( rand() < RAND_MAX / 128 )
				_soundRender.PlayOnce(SoundTemplate::Hit1, obj.GetPos());
			else if( rand() < RAND_MAX / 128 )
				_soundRender.PlayOnce(SoundTemplate::Hit3, obj.GetPos());
			else if( rand() < RAND_MAX / 128 )
				_soundRender.PlayOnce(SoundTemplate::Hit5, obj.GetPos());
		}
	}
}

void SoundHarness::OnContact(vec2d pos, float np, float tp)
{
	float nd = (np + tp)/60;

	if( nd > 3 )
	{
		if( nd > 10 )
			_soundRender.PlayOnce(SoundTemplate::Impact2, pos);
		else
			_soundRender.PlayOnce(SoundTemplate::Impact1, pos);
	}
	else if( tp > 10 )
	{
		_soundRender.PlayOnce(SoundTemplate::Slide1, pos);
	}
}

void SoundHarness::OnShoot(GC_Turret &obj)
{
	if (GC_TurretCannon::GetTypeStatic() == obj.GetType())
		_soundRender.PlayOnce(SoundTemplate::Shoot, obj.GetPos());
	else if (GC_TurretRocket::GetTypeStatic() == obj.GetType())
		_soundRender.PlayOnce(SoundTemplate::RocketShoot, obj.GetPos());
	else if (GC_TurretGauss::GetTypeStatic() == obj.GetType())
		_soundRender.PlayOnce(SoundTemplate::Bolt, obj.GetPos());
}

void SoundHarness::OnFireStateChange(GC_Turret &obj)
{
	if (obj.GetFire())
	{
		if (GC_TurretMinigun::GetTypeStatic() == obj.GetType())
			_turretFire.emplace(static_cast<const GC_Turret*>(&obj), _soundRender.CreateLopped(SoundTemplate::MinigunFire));
	}
	else
	{
		_turretFire.erase(&obj);
	}
}

void SoundHarness::OnStateChange(GC_Turret &turret)
{
	switch (turret.GetState())
	{
		case TS_WAKING_UP:
			_soundRender.PlayOnce(SoundTemplate::TuretWakeUp, turret.GetPos());
			break;
		case TS_WAKING_DOWN:
			_soundRender.PlayOnce(SoundTemplate::TuretWakeDown, turret.GetPos());
			break;
		case TS_ATACKING:
			_soundRender.PlayOnce(SoundTemplate::TargetLock, turret.GetPos());
			break;
		default:
			break;
	}
}

void SoundHarness::OnRotationStateChange(GC_Turret &turret)
{
	auto existing = _turretRotate.find(&turret);
	if (_turretRotate.end() != existing)
	{
		if (turret.GetRotationState() == RS_STOPPED)
			_turretRotate.erase(existing);
	}
	else
	{
		if (turret.GetRotationState() != RS_STOPPED)
			_turretRotate.emplace(&turret, _soundRender.CreateLopped(SoundTemplate::TowerRotate));
	}
}

void SoundHarness::OnLight(GC_Vehicle &obj)
{
	_soundRender.PlayOnce(SoundTemplate::LightSwitch, obj.GetPos());
}

void SoundHarness::OnGameFinished()
{
	// FIXME: play at no specific position
	_soundRender.PlayOnce(SoundTemplate::Limit, vec2d(0,0));
}

void SoundHarness::OnKill(GC_Object &obj)
{
	ObjectType type = obj.GetType();
	if (GC_Rocket::GetTypeStatic() == type)
		_attached.erase(static_cast<GC_Rocket*>(&obj));
	else if (GC_Tank_Light::GetTypeStatic() == type)
		_vehicleMove.erase(static_cast<const GC_Vehicle*>(&obj));
}

void SoundHarness::OnNewObject(GC_Object &obj)
{
	ObjectType type = obj.GetType();
	if (GC_ExplosionBig::GetTypeStatic() == type)
		_soundRender.PlayOnce(SoundTemplate::BoomBig, static_cast<const GC_Actor &>(obj).GetPos());
	else if (GC_ExplosionStandard::GetTypeStatic() == type)
		_soundRender.PlayOnce(SoundTemplate::BoomStandard, static_cast<const GC_Actor &>(obj).GetPos());
	else if (GC_Rocket::GetTypeStatic() == type)
		_attached.emplace(static_cast<const GC_Actor*>(&obj),
			CreatePlayingLooped(_soundRender, SoundTemplate::RocketFly,
								[&](Sound &s){ s.SetPos(static_cast<const GC_Actor &>(obj).GetPos()); }));
	else if (GC_Tank_Light::GetTypeStatic() == type)
		_vehicleMove.emplace(static_cast<const GC_Vehicle*>(&obj),
							 CreatePlayingLooped(_soundRender, SoundTemplate::TankMove,
												 [&](Sound &s){ SetupVehicleMoveSound(static_cast<const GC_Vehicle &>(obj), s); }));
}
