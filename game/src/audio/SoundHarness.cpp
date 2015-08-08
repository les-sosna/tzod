#include "SoundHarness.h"
#include "SoundRender.h"
#include <gc/Crate.h>
#include <gc/Explosion.h>
#include <gc/Pickup.h>
#include <gc/RigidBody.h>
#include <gc/Projectiles.h>
#include <gc/Turrets.h>
#include <gc/Vehicle.h>
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
		{GC_pu_Health::GetTypeStatic(), SND_Pickup},
		{GC_pu_Shield::GetTypeStatic(), SND_Inv},
		{GC_pu_Shock::GetTypeStatic(), SND_ShockActivate},
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
			_attached.emplace(&obj, CreatePlayingLooped(_soundRender, SND_B_Loop, [&](Sound &s){ s.SetPos(obj.GetPos()); }));
			_soundRender.PlayOnce(SND_B_Start, obj.GetPos());
		}
	}
	else if (auto weapon = dynamic_cast<GC_Weapon*>(&obj))
	{
		_soundRender.PlayOnce(SND_w_Pickup, obj.GetPos());
		_weaponRotate.emplace(weapon, _soundRender.CreateLopped(SND_TowerRotate));
		if (GC_Weap_Minigun::GetTypeStatic() == type)
			_weaponFire.emplace(weapon, _soundRender.CreateLopped(SND_MinigunFire));
		else if (GC_Weap_Zippo::GetTypeStatic() == type)
			_weaponFire.emplace(weapon, _soundRender.CreateLopped(SND_RamEngine));
		else if (GC_Weap_Ram::GetTypeStatic() == type)
			_weaponFire.emplace(weapon, _soundRender.CreateLopped(SND_RamEngine));
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
	_soundRender.PlayOnce(SND_puRespawn, obj.GetPos());
}

void SoundHarness::OnDisappear(GC_Pickup &obj)
{
	if (GC_pu_Booster::GetTypeStatic() == obj.GetType())
	{
		_soundRender.PlayOnce(SND_B_End, obj.GetPos());
	}
}

void SoundHarness::OnHit(GC_Projectile &obj, GC_RigidBodyStatic &target, vec2d hit)
{
	static std::unordered_map<ObjectType, SoundTemplate> sounds = {
		{GC_TankBullet::GetTypeStatic(), SND_BoomBullet},
		{GC_PlazmaClod::GetTypeStatic(), SND_PlazmaHit},
		{GC_BfgCore::GetTypeStatic(), SND_BfgFlash},
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
			_soundRender.PlayOnce((rand() % 2) ? SND_AC_Hit2 : SND_AC_Hit3, hit);
		else
			_soundRender.PlayOnce(SND_AC_Hit1, hit);
	}
	else if (GC_Disk::GetTypeStatic() == type)
	{
		if (static_cast<const GC_Disk&>(obj).GetBounces())
			_soundRender.PlayOnce(SND_DiskHit, hit);
		else
			_soundRender.PlayOnce(SND_BoomBullet, hit);
	}
}

void SoundHarness::OnShoot(GC_ProjectileBasedWeapon &obj)
{
	static std::unordered_map<ObjectType, SoundTemplate> sounds = {
		{GC_Weap_AutoCannon::GetTypeStatic(), SND_ACShoot},
		{GC_Weap_Cannon::GetTypeStatic(), SND_Shoot},
		{GC_Weap_Gauss::GetTypeStatic(), SND_Bolt},
		{GC_Weap_Plazma::GetTypeStatic(), SND_PlazmaFire},
		{GC_Weap_Ripper::GetTypeStatic(), SND_DiskFire},
		{GC_Weap_RocketLauncher::GetTypeStatic(), SND_RocketShoot},
	};
	auto found = sounds.find(obj.GetType());
	if (sounds.end() != found)
	{
		_soundRender.PlayOnce(found->second, obj.GetPos());
	}
	else if (GC_Weap_BFG::GetTypeStatic() == obj.GetType())
	{
		if( obj.GetNumShots() )
			_soundRender.PlayOnce(SND_BfgFire, obj.GetPos());
		else if( !obj.GetBooster() )
			_soundRender.PlayOnce(SND_BfgInit, obj.GetPos());
	}
}

void SoundHarness::OnOwnerDamage(GC_pu_Shield &obj, DamageDesc &dd)
{
	_soundRender.PlayOnce(rand() % 2 ? SND_InvHit1 : SND_InvHit2, obj.GetPos());
}

void SoundHarness::OnExpiring(GC_pu_Shield &obj)
{
	_soundRender.PlayOnce(SND_InvEnd, obj.GetPos());
}

void SoundHarness::OnDestroy(GC_RigidBodyStatic &obj, const DamageDesc &dd)
{
	ObjectType type = obj.GetType();
	if (GC_Crate::GetTypeStatic() == type)
		_soundRender.PlayOnce(SND_WallDestroy, obj.GetPos());
	else if (GC_Wall::GetTypeStatic() == type)
		_soundRender.PlayOnce(SND_WallDestroy, obj.GetPos());
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
				_soundRender.PlayOnce(SND_Hit1, obj.GetPos());
			else if( rand() < RAND_MAX / 128 )
				_soundRender.PlayOnce(SND_Hit3, obj.GetPos());
			else if( rand() < RAND_MAX / 128 )
				_soundRender.PlayOnce(SND_Hit5, obj.GetPos());
		}
	}
}

void SoundHarness::OnContact(vec2d pos, float np, float tp)
{
	float nd = (np + tp)/60;

	if( nd > 3 )
	{
		if( nd > 10 )
			_soundRender.PlayOnce(SND_Impact2, pos);
		else
			_soundRender.PlayOnce(SND_Impact1, pos);
	}
	else if( tp > 10 )
	{
		_soundRender.PlayOnce(SND_Slide1, pos);
	}
}

void SoundHarness::OnShoot(GC_Turret &obj)
{
	if (GC_TurretCannon::GetTypeStatic() == obj.GetType())
		_soundRender.PlayOnce(SND_Shoot, obj.GetPos());
	else if (GC_TurretRocket::GetTypeStatic() == obj.GetType())
		_soundRender.PlayOnce(SND_RocketShoot, obj.GetPos());
	else if (GC_TurretGauss::GetTypeStatic() == obj.GetType())
		_soundRender.PlayOnce(SND_Bolt, obj.GetPos());
}

void SoundHarness::OnFireStateChange(GC_Turret &obj)
{
	if (obj.GetFire())
	{
		if (GC_TurretMinigun::GetTypeStatic() == obj.GetType())
			_turretFire.emplace(static_cast<const GC_Turret*>(&obj), _soundRender.CreateLopped(SND_MinigunFire));
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
			_soundRender.PlayOnce(SND_TuretWakeUp, turret.GetPos());
			break;
		case TS_WAKING_DOWN:
			_soundRender.PlayOnce(SND_TuretWakeDown, turret.GetPos());
			break;
		case TS_ATACKING:
			_soundRender.PlayOnce(SND_TargetLock, turret.GetPos());
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
			_turretRotate.emplace(&turret, _soundRender.CreateLopped(SND_TowerRotate));
	}
}

void SoundHarness::OnLight(GC_Vehicle &obj)
{
	_soundRender.PlayOnce(SND_LightSwitch, obj.GetPos());
}

void SoundHarness::OnGameFinished()
{
	// FIXME: play at no specific position
	_soundRender.PlayOnce(SND_Limit, vec2d(0,0));
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
		_soundRender.PlayOnce(SND_BoomBig, static_cast<const GC_Actor &>(obj).GetPos());
	else if (GC_ExplosionStandard::GetTypeStatic() == type)
		_soundRender.PlayOnce(SND_BoomStandard, static_cast<const GC_Actor &>(obj).GetPos());
	else if (GC_Rocket::GetTypeStatic() == type)
		_attached.emplace(static_cast<const GC_Actor*>(&obj),
			CreatePlayingLooped(_soundRender, SND_RocketFly,
								[&](Sound &s){ s.SetPos(static_cast<const GC_Actor &>(obj).GetPos()); }));
	else if (GC_Tank_Light::GetTypeStatic() == type)
		_vehicleMove.emplace(static_cast<const GC_Vehicle*>(&obj),
							 CreatePlayingLooped(_soundRender, SND_TankMove,
												 [&](Sound &s){ SetupVehicleMoveSound(static_cast<const GC_Vehicle &>(obj), s); }));
}
