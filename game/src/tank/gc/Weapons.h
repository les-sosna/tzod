#pragma once

#include "WeaponBase.h"


class GC_Weap_RocketLauncher : public GC_ProjectileBasedWeapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_RocketLauncher);

public:
	GC_Weap_RocketLauncher(World &world);
	GC_Weap_RocketLauncher(FromFile);
	
	enum
	{
		SERIES_LENGTH = 6
	};

	// GC_ProctileBasedWeapon
	virtual bool GetContinuousSeries() const { return !GetAdvanced(); }
	virtual float GetFireEffectTime() const override { return 0.1f; }
	virtual float GetReloadTime() const override { return GetAdvanced() ? 0.13f : 2.0f; }
	virtual unsigned int GetSeriesLength() const override { return GetAdvanced() ? 1 : SERIES_LENGTH; }
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
	GC_Weap_AutoCannon(World &world);
	GC_Weap_AutoCannon(FromFile);
	
	// GC_ProjectileBasedWeapon
	virtual float GetFireEffectTime() const override { return 0.2f; }
	virtual float GetReloadTime() const override { return 3.7f; }
	virtual unsigned int GetSeriesLength() const override { return 30; }
	virtual float GetSeriesReloadTime() const override { return 0.135f; }

	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const override;
	virtual void SetupAI(AIWEAPSETTINGS *pSettings) override;

protected:
	virtual void OnShoot(World &world) override;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Cannon : public GC_ProjectileBasedWeapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Cannon);

public:
	GC_Weap_Cannon(World &world);
	GC_Weap_Cannon(FromFile);

	// GC_ProjectileBasedWeapon
	virtual float GetFireEffectTime() const override { return 0.2f; }
	virtual float GetReloadTime() const { return 0.9f; }

	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const;
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);

	// GC_Pickup
	virtual void Attach(World &world, GC_Actor *actor);

	// GC_Object
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStep(World &world, float dt);

protected:
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
	GC_Weap_Plazma(World &world);
	GC_Weap_Plazma(FromFile);

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
	GC_Weap_Gauss(World &world);
	GC_Weap_Gauss(FromFile);

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
	GC_Weap_Ram(World &world);
	GC_Weap_Ram(FromFile);
	virtual ~GC_Weap_Ram();
	
	float GetFuel() const { return _fuel; }
	float GetFuelMax() const { return _fuel_max; }

	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const;
	virtual void Fire(World &world, bool fire);
	virtual void SetAdvanced(World &world, bool advanced);
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);

	// GC_Pickup
	virtual void Attach(World &world, GC_Actor *actor);
	virtual void Detach(World &world);
	
	// GC_Object
	virtual void Kill(World &world);
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStep(World &world, float dt);

private:
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
	GC_Weap_BFG(World &world);
	GC_Weap_BFG(FromFile);

	// GC_ProjectileBasedWeapon
	virtual bool GetContinuousSeries() const { return true; }
	virtual float GetFireEffectTime() const { return 0; }
	virtual float GetReloadTime() const { return 1.1f; }
	virtual unsigned int GetSeriesLength() const override { return 2; }
	virtual float GetSeriesReloadTime() const override { return 0.7f; }
	
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
	GC_Weap_Ripper(World &world);
	GC_Weap_Ripper(FromFile);

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
	GC_Weap_Minigun(World &world);
	GC_Weap_Minigun(FromFile);
	virtual ~GC_Weap_Minigun();

	float GetHeat(const World &world) const;

	// GC_ProjectileBasedWeapon
	virtual float GetFireEffectTime() const override { return 0.1f; }
	virtual float GetReloadTime() const { return GetAdvanced() ? 0.02f : 0.04f; }
	
	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const;
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	
	// GC_Pickup
	virtual void Attach(World &world, GC_Actor *actor);
	virtual void Detach(World &world);

	// GC_Object
	virtual void Kill(World &world);
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStep(World &world, float dt);

protected:
	virtual void OnShoot(World &world) override;

private:
	ObjPtr<GC_Sound> _sound;
	float _heat = 0;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Zippo : public GC_ProjectileBasedWeapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Zippo);

public:
	GC_Weap_Zippo(World &world);
	GC_Weap_Zippo(FromFile);
	virtual ~GC_Weap_Zippo();
	
	// GC_ProjectileBasedWeapon
	virtual float GetFireEffectTime() const { return 0; }
	virtual float GetReloadTime() const { return 0.02f; }

	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const;
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);

	// GC_Pickup
	virtual void Attach(World &world, GC_Actor *actor);
	virtual void Detach(World &world);

	// GC_Object
	virtual void Kill(World &world);
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStep(World &world, float dt);
	
protected:
	virtual void OnShoot(World &world) override;

private:
	ObjPtr<GC_Sound> _sound;
	float _timeBurn;
	float _heat = 0;
};

