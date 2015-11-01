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
    bool GetContinuousSeries() const override { return !GetBooster(); }
    float GetFireEffectTime() const override { return 0.1f; }
    float GetReloadTime() const override { return GetBooster() ? 0.13f : 2.0f; }
    unsigned int GetSeriesLength() const override { return GetBooster() ? 1 : SERIES_LENGTH; }
    float GetSeriesReloadTime() const override { return 0.13f; }

	// GC_Weapon
    void SetupAI(AIWEAPSETTINGS *pSettings) override;
    void AdjustVehicleClass(VehicleClass &vc) const override;

protected:
    void OnShoot(World &world) override;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_AutoCannon : public GC_ProjectileBasedWeapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_AutoCannon);

public:
	explicit GC_Weap_AutoCannon(vec2d pos);
	explicit GC_Weap_AutoCannon(FromFile);

	// GC_ProjectileBasedWeapon
	float GetFireEffectTime() const override { return 0.2f; }
	float GetReloadTime() const override { return GetBooster() ? 0.135f : 3.7f; }
	unsigned int GetSeriesLength() const override { return GetBooster() ? 1 : 30; }
	float GetSeriesReloadTime() const override { return 0.135f; }

	// GC_Weapon
	void AdjustVehicleClass(VehicleClass &vc) const override;
	void SetBooster(World &world, GC_pu_Booster *booster) override;
	void SetupAI(AIWEAPSETTINGS *pSettings) override;

protected:
	void OnShoot(World &world) override;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Cannon : public GC_ProjectileBasedWeapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Cannon);

public:
	explicit GC_Weap_Cannon(vec2d pos);
	explicit GC_Weap_Cannon(FromFile);

	// GC_ProjectileBasedWeapon
    float GetFireEffectTime() const override { return 0.2f; }
    float GetReloadTime() const override { return 0.9f; }

	// GC_Weapon
    void AdjustVehicleClass(VehicleClass &vc) const override;
    void SetupAI(AIWEAPSETTINGS *pSettings) override;

	// GC_Object
    void Serialize(World &world, SaveFile &f) override;
    void TimeStep(World &world, float dt) override;

protected:
    void OnAttached(World &world, GC_Vehicle &vehicle) override;
    void OnShoot(World &world) override;

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
    float GetFireEffectTime() const override { return 0.2f; }
    float GetReloadTime() const override { return 0.3f; }

	// GC_Weapon
    void AdjustVehicleClass(VehicleClass &vc) const override;
    void SetupAI(AIWEAPSETTINGS *pSettings) override;

protected:
    void OnShoot(World &world) override;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Gauss : public GC_ProjectileBasedWeapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Gauss);

public:
	explicit GC_Weap_Gauss(vec2d pos);
	explicit GC_Weap_Gauss(FromFile);

	// GC_ProjectileBasedWeapon
    float GetFireEffectTime() const override { return 0.15f; }
    float GetReloadTime() const override { return 1.3f; }

	// GC_Weapon
    void AdjustVehicleClass(VehicleClass &vc) const override;
    void SetupAI(AIWEAPSETTINGS *pSettings) override;

protected:
    void OnShoot(World &world) override;
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
    void AdjustVehicleClass(VehicleClass &vc) const override;
    void Fire(World &world, bool fire) override;
    bool GetFire() const override;
    void SetBooster(World &world, GC_pu_Booster *booster) override;
    void SetupAI(AIWEAPSETTINGS *pSettings) override;

	// GC_Pickup
    void Detach(World &world) override;

	// GC_Object
    void Serialize(World &world, SaveFile &f) override;
    void TimeStep(World &world, float dt) override;

protected:
    void OnAttached(World &world, GC_Vehicle &vehicle) override;

private:
	vec2d GetEngineLightPos() const;
    void OnUpdateView(World &world) override;
	ObjPtr<GC_Light> _engineLight;

	float _fuel;
	float _fuel_max;
	float _fuel_consumption_rate;
	float _fuel_recuperation_rate;
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
    bool GetContinuousSeries() const override { return true; }
    float GetFireEffectTime() const override { return 0; }
    float GetReloadTime() const override { return 1.1f; }
    unsigned int GetSeriesLength() const override { return 2; }
    float GetSeriesReloadTime() const override { return GetBooster() ? 0.0f : 0.7f; }

	// GC_Weapon
    void AdjustVehicleClass(VehicleClass &vc) const override;
    void SetupAI(AIWEAPSETTINGS *pSettings) override;

protected:
	void OnShoot(World &world) override;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Ripper : public GC_ProjectileBasedWeapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Ripper);

public:
	explicit GC_Weap_Ripper(vec2d pos);
	explicit GC_Weap_Ripper(FromFile);

	// GC_ProjectileBasedWeapon
    float GetFireEffectTime() const override { return 0; }
    float GetReloadTime() const override { return 0.5f; }

	// GC_Weapon
    void AdjustVehicleClass(VehicleClass &vc) const override;
    void SetupAI(AIWEAPSETTINGS *pSettings) override;

protected:
    void OnShoot(World &world) override;
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
    float GetFireEffectTime() const override { return 0.1f; }
    float GetReloadTime() const override { return GetBooster() ? 0.02f : 0.04f; }

	// GC_Weapon
    void AdjustVehicleClass(VehicleClass &vc) const override;
    void SetupAI(AIWEAPSETTINGS *pSettings) override;

	// GC_Pickup
    void Detach(World &world) override;

	// GC_Object
    void Serialize(World &world, SaveFile &f) override;

protected:
    void OnShoot(World &world) override;

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
    float GetFireEffectTime() const override { return 0; }
    float GetReloadTime() const override { return 0.02f; }

	// GC_Weapon
    void AdjustVehicleClass(VehicleClass &vc) const override;
    void SetupAI(AIWEAPSETTINGS *pSettings) override;

	// GC_Pickup
    void Detach(World &world) override;

	// GC_Object
    void Serialize(World &world, SaveFile &f) override;
    void TimeStep(World &world, float dt) override;

protected:
    void OnShoot(World &world) override;

private:
	float _timeBurn;
	float _heat = 0;
};

