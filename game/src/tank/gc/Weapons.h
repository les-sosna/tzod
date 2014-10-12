#pragma once

#include "WeaponBase.h"


class GC_Weap_RocketLauncher : public GC_ProjectileBasedWeapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_RocketLauncher);

public:
	GC_Weap_RocketLauncher(World &world);
	GC_Weap_RocketLauncher(FromFile);

	// GC_Weapon
	virtual void Fire(World &world, bool fire);
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void AdjustVehicleClass(VehicleClass &vc) const;

	// GC_Pickup
	virtual void Attach(World &world, GC_Actor *actor);
	virtual void Detach(World &world);

	// GC_Object
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStep(World &world, float dt);

protected:
	virtual float GetReloadTime() const override { return 2.0f; }
	virtual float GetFireEffectTime() const override { return 0.1f; }
	virtual void OnShoot(World &world) override;

private:
	void Shoot1(World &world);
	float _time_shot;
	int _nshots_total;
	int _nshots;
	bool _firing;
	bool _reloaded;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_AutoCannon : public GC_ProjectileBasedWeapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_AutoCannon);

public:
	GC_Weap_AutoCannon(World &world);
	GC_Weap_AutoCannon(FromFile);
	virtual ~GC_Weap_AutoCannon();
	
	int GetShots() const { return _nshots; }
	int GetShotsTotal() const { return _nshots_total; }

	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const;
	virtual void Fire(World &world, bool fire);
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);

	// GC_Pickup
	virtual void Attach(World &world, GC_Actor *actor);
	virtual void Detach(World &world);

	// GC_Object
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStep(World &world, float dt);

protected:
	virtual float GetFireEffectTime() const override { return 0.2f; }
	virtual float GetReloadTime() const { return 3.7f; }
	virtual void OnShoot(World &world) override;

private:
	float _time_shot;
	int _nshots_total;
	int _nshots;
	bool _firing;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Cannon : public GC_ProjectileBasedWeapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Cannon);

public:
	virtual void Attach(World &world, GC_Actor *actor);

	GC_Weap_Cannon(World &world);
	GC_Weap_Cannon(FromFile);
	virtual ~GC_Weap_Cannon();

	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const;
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);

	// GC_Object
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStep(World &world, float dt);

protected:
	virtual float GetFireEffectTime() const override { return 0.2f; }
	virtual float GetReloadTime() const { return 0.9f; }
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
	virtual ~GC_Weap_Plazma();

	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const;
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);

protected:
	virtual float GetFireEffectTime() const override { return 0.2f; }
	virtual float GetReloadTime() const { return 0.3f; }
	virtual void OnShoot(World &world) override;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Gauss : public GC_ProjectileBasedWeapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Gauss);

public:
	GC_Weap_Gauss(World &world);
	GC_Weap_Gauss(FromFile);
	virtual ~GC_Weap_Gauss();

	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const;
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);

protected:
	virtual float GetFireEffectTime() const override { return 0.15f; }
	virtual float GetReloadTime() const { return 1.3f; }
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
	virtual ~GC_Weap_BFG();

	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const;
	virtual void Fire(World &world, bool fire);
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	
	// GC_Pickup
	virtual void Attach(World &world, GC_Actor *actor);

	// GC_Object
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStep(World &world, float dt);

protected:
	virtual float GetFireEffectTime() const { return 0; }
	virtual float GetReloadTime() const { return 1.1f; }
	virtual void OnShoot(World &world) override;

private:
	void Shoot1(World &world);
	float _time_ready;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Ripper : public GC_ProjectileBasedWeapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Ripper);

public:
	GC_Weap_Ripper(World &world);
	GC_Weap_Ripper(FromFile);
	virtual ~GC_Weap_Ripper();
		
	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const;
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	
protected:
	virtual float GetFireEffectTime() const { return 0; }
	virtual float GetReloadTime() const { return 0.5f; }
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
	virtual float GetFireEffectTime() const override { return 0.1f; }
	virtual float GetReloadTime() const { return GetAdvanced() ? 0.02f : 0.04f; }
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
	virtual float GetFireEffectTime() const { return 0; }
	virtual float GetReloadTime() const { return 0.02f; }
	virtual void OnShoot(World &world) override;

private:
	ObjPtr<GC_Sound> _sound;
	float _timeBurn;
	float _heat = 0;
};

// end of file
