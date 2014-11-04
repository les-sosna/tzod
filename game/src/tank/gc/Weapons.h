#pragma once

#include "WeaponBase.h"


class GC_Weap_RocketLauncher : public GC_ProjectileBasedWeapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_RocketLauncher);

public:
	explicit GC_Weap_RocketLauncher(vec2d pos);
	explicit GC_Weap_RocketLauncher(FromFile);
	
	enum
	{
		SERIES_LENGTH = 6
	};

	// GC_ProctileBasedWeapon
	virtual bool GetContinuousSeries() const { return !GetBooster(); }
	virtual float GetFireEffectTime() const override { return 0.1f; }
	virtual float GetReloadTime() const override { return GetBooster() ? 0.13f : 2.0f; }
	virtual unsigned int GetSeriesLength() const override { return GetBooster() ? 1 : SERIES_LENGTH; }
	virtual float GetSeriesReloadTime() const override { return 0.13f; }
	
	// GC_Weapon
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void AdjustVehicleClass(VehicleClass &vc) const;

protected:
	virtual void OnShoot(World &world) override;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_AutoCannon : public GC_ProjectileBasedWeapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_AutoCannon);

public:
	explicit GC_Weap_AutoCannon(vec2d pos);
	explicit GC_Weap_AutoCannon(FromFile);
	
	// GC_ProjectileBasedWeapon
	virtual float GetFireEffectTime() const override { return 0.2f; }
	virtual float GetReloadTime() const override { return GetBooster() ? 0.135f : 3.7f; }
	virtual unsigned int GetSeriesLength() const override { return GetBooster() ? 1 : 30; }
	virtual float GetSeriesReloadTime() const override { return 0.135f; }

	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const override;
	virtual void SetBooster(World &world, GC_pu_Booster *booster) override;
	virtual void SetupAI(AIWEAPSETTINGS *pSettings) override;

protected:
	virtual void OnShoot(World &world) override;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Cannon : public GC_ProjectileBasedWeapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Cannon);

public:
	explicit GC_Weap_Cannon(vec2d pos);
	explicit GC_Weap_Cannon(FromFile);

	// GC_ProjectileBasedWeapon
	virtual float GetFireEffectTime() const override { return 0.2f; }
	virtual float GetReloadTime() const { return 0.9f; }

	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const;
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);

	// GC_Object
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStep(World &world, float dt);

protected:
	virtual void OnAttached(World &world, GC_Vehicle &vehicle);
	virtual void OnShoot(World &world) override;

private:
	float _time_smoke;
	float _time_smoke_dt;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Plazma : public GC_ProjectileBasedWeapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Plazma);
	
public:
	explicit GC_Weap_Plazma(vec2d pos);
	explicit GC_Weap_Plazma(FromFile);

	// GC_ProjectileBasedWeapon
	virtual float GetFireEffectTime() const override { return 0.2f; }
	virtual float GetReloadTime() const { return 0.3f; }

	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const;
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);

protected:
	virtual void OnShoot(World &world) override;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Gauss : public GC_ProjectileBasedWeapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Gauss);
	
public:
	explicit GC_Weap_Gauss(vec2d pos);
	explicit GC_Weap_Gauss(FromFile);

	// GC_ProjectileBasedWeapon
	virtual float GetFireEffectTime() const override { return 0.15f; }
	virtual float GetReloadTime() const { return 1.3f; }
	
	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const;
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);

protected:
	virtual void OnShoot(World &world) override;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Ram : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Ram);
	
public:
	explicit GC_Weap_Ram(vec2d pos);
	explicit GC_Weap_Ram(FromFile);
	virtual ~GC_Weap_Ram();
	
	float GetFuel() const { return _fuel; }
	float GetFuelMax() const { return _fuel_max; }

	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const override;
	virtual void Fire(World &world, bool fire) override;
	virtual void SetBooster(World &world, GC_pu_Booster *booster) override;
	virtual void SetupAI(AIWEAPSETTINGS *pSettings) override;

	// GC_Pickup
	virtual void Detach(World &world) override;
	
	// GC_Object
	virtual void Kill(World &world) override;
	virtual void Serialize(World &world, SaveFile &f) override;
	virtual void TimeStep(World &world, float dt) override;

protected:
	virtual void OnAttached(World &world, GC_Vehicle &vehicle) override;

private:
	vec2d GetEngineLightPos() const;
	virtual void OnUpdateView(World &world);
	ObjPtr<GC_Sound> _engineSound;
	ObjPtr<GC_Light> _engineLight;
	
	float _fuel;
	float _fuel_max;
	float _fuel_consumption_rate;
	float _fuel_recuperation_rate;
	int _firingCounter;
	bool _bReady;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_BFG : public GC_ProjectileBasedWeapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_BFG);
	
public:
	explicit GC_Weap_BFG(vec2d pos);
	explicit GC_Weap_BFG(FromFile);

	// GC_ProjectileBasedWeapon
	virtual bool GetContinuousSeries() const { return true; }
	virtual float GetFireEffectTime() const { return 0; }
	virtual float GetReloadTime() const { return 1.1f; }
	virtual unsigned int GetSeriesLength() const override { return 2; }
	virtual float GetSeriesReloadTime() const override { return GetBooster() ? 0.0f : 0.7f; }
	
	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const;
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	
protected:
	virtual void OnShoot(World &world) override;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Ripper : public GC_ProjectileBasedWeapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Ripper);

public:
	explicit GC_Weap_Ripper(vec2d pos);
	explicit GC_Weap_Ripper(FromFile);

	// GC_ProjectileBasedWeapon
	virtual float GetFireEffectTime() const { return 0; }
	virtual float GetReloadTime() const { return 0.5f; }
	
	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const;
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	
protected:
	virtual void OnShoot(World &world) override;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Minigun : public GC_ProjectileBasedWeapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Minigun);

public:
	explicit GC_Weap_Minigun(vec2d pos);
	explicit GC_Weap_Minigun(FromFile);
	virtual ~GC_Weap_Minigun();

	float GetHeat(const World &world) const;

	// GC_ProjectileBasedWeapon
	virtual float GetFireEffectTime() const override { return 0.1f; }
	virtual float GetReloadTime() const override { return GetBooster() ? 0.02f : 0.04f; }
	
	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const override;
	virtual void SetupAI(AIWEAPSETTINGS *pSettings) override;
	
	// GC_Pickup
	virtual void Detach(World &world) override;

	// GC_Object
	virtual void Serialize(World &world, SaveFile &f) override;

protected:
	virtual void OnShoot(World &world) override;

private:
	float _heat = 0;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Zippo : public GC_ProjectileBasedWeapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Zippo);

public:
	explicit GC_Weap_Zippo(vec2d pos);
	explicit GC_Weap_Zippo(FromFile);
	virtual ~GC_Weap_Zippo();
	
	// GC_ProjectileBasedWeapon
	virtual float GetFireEffectTime() const { return 0; }
	virtual float GetReloadTime() const { return 0.02f; }

	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const;
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);

	// GC_Pickup
	virtual void Detach(World &world);

	// GC_Object
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStep(World &world, float dt);
	
protected:
	virtual void OnShoot(World &world) override;

private:
	float _timeBurn;
	float _heat = 0;
};

