// Weapons.h

#pragma once

#include "Pickup.h"

struct VehicleClass;

struct AIWEAPSETTINGS
{
	float fMaxAttackAngleCos;
	float fProjectileSpeed;
	float fAttackRadius_min;
	float fAttackRadius_max;
	float fAttackRadius_crit;  // if you closer than critical distance you may damage your self
	float fDistanceMultipler;  // applies when traveling through brick walls
	bool  bNeedOutstrip;       // false if the projectile speed is unlimited
};

class GC_Weapon : public GC_Pickup
{
protected:
	class MyPropertySet : public GC_Pickup::MyPropertySet
	{
		typedef GC_Pickup::MyPropertySet BASE;
		ObjectProperty _propTimeStay;

	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void MyExchange(World &world, bool applyToObject);
	};
	virtual PropertySet* NewPropertySet();

protected:
	ObjPtr<GC_Light>    _fireLight;
	vec2d _fePos;
	vec2d _feOrient;
	float _feTime;
	bool _advanced; // weapon has booster attached

public:
	float _time;
	float _timeStay;
	float _timeReload;
	float _lastShotTimestamp;

	float    _angle;
	Rotator  _rotatorWeap;

	ObjPtr<GC_Sound>      _rotateSound;

public:
	GC_Weapon(World &world);
	GC_Weapon(FromFile);
	virtual ~GC_Weapon();
	
	bool GetAdvanced() const { return _advanced;     }
	GC_RigidBodyStatic* GetCarrier() const { return reinterpret_cast<GC_RigidBodyStatic *>(GC_Pickup::GetCarrier()); }
	float GetLastShotTimestamp() const { return _lastShotTimestamp; }
	float GetFirePointOffset() const { return _fePos.y; }
	
	virtual void Fire(World &world, bool fire) = 0;
	virtual void SetAdvanced(World &world, bool advanced) { _advanced = advanced; }
	virtual void SetupAI(AIWEAPSETTINGS *pSettings) = 0;
	virtual void AdjustVehicleClass(VehicleClass &vc) const = 0;

	// GC_Pickup
	virtual float GetDefaultRespawnTime() const override { return 6.0f; }
	virtual AIPRIORITY GetPriority(World &world, const GC_Vehicle &veh) const override;
	virtual void Attach(World &world, GC_Actor *actor) override;
	virtual void Detach(World &world) override;
	
	// GC_Object
	virtual void Kill(World &world);
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStep(World &world, float dt);

private:
	virtual void OnUpdateView(World &world) {};
	void ProcessRotate(World &world, float dt);

#ifdef NETWORK_DEBUG
/*	virtual DWORD checksum(void) const
	{
		DWORD cs = reinterpret_cast<const DWORD&>(_angleReal)
			^ reinterpret_cast<const DWORD&>(_timeReload)
			^ reinterpret_cast<const DWORD&>(_timeStay);
		return GC_Pickup::checksum() ^ cs;
	}*/
#endif
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_RocketLauncher : public GC_Weapon
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

private:
	void Shoot(World &world);
	float _time_shot;
	int _nshots_total;
	int _nshots;
	bool _firing;
	bool _reloaded;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_AutoCannon : public GC_Weapon
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
	
private:
	float _time_shot;
	int _nshots_total;
	int _nshots;
	bool _firing;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Cannon : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Cannon);

public:
	virtual void Attach(World &world, GC_Actor *actor);

	GC_Weap_Cannon(World &world);
	GC_Weap_Cannon(FromFile);
	virtual ~GC_Weap_Cannon();

	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const;
	virtual void Fire(World &world, bool fire);
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);

	// GC_Object
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStep(World &world, float dt);

private:
	float _time_smoke;
	float _time_smoke_dt;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Plazma : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Plazma);

public:
	GC_Weap_Plazma(World &world);
	GC_Weap_Plazma(FromFile);
	virtual ~GC_Weap_Plazma();

	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const;
	virtual void Fire(World &world, bool fire);
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);

	// GC_Pickup
	virtual void Attach(World &world, GC_Actor *actor);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Gauss : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Gauss);

public:
	GC_Weap_Gauss(World &world);
	GC_Weap_Gauss(FromFile);
	virtual ~GC_Weap_Gauss();

	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const;
	virtual void Fire(World &world, bool fire);
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	
	// GC_Pickup
	virtual void Attach(World &world, GC_Actor *actor);
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

class GC_Weap_BFG : public GC_Weapon
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

private:
	void Shoot(World &world);
	float _time_ready;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Ripper : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Ripper);

public:
	GC_Weap_Ripper(World &world);
	GC_Weap_Ripper(FromFile);
	virtual ~GC_Weap_Ripper();
	
	bool IsReady() const { return GetCarrier() && _time > _timeReload; }
	
	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const;
	virtual void Fire(World &world, bool fire);
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	
	// GC_Pickup
	virtual void Attach(World &world, GC_Actor *actor);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Minigun : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Minigun);

public:
	GC_Weap_Minigun(World &world);
	GC_Weap_Minigun(FromFile);
	virtual ~GC_Weap_Minigun();

	bool GetFire() const { return _bFire; }
	float GetFireTime() const { return _timeFire; }

	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const;
	virtual void Fire(World &world, bool fire);
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	
	// GC_Pickup
	virtual void Attach(World &world, GC_Actor *actor);
	virtual void Detach(World &world);

	// GC_Object
	virtual void Kill(World &world);
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStep(World &world, float dt);

private:
	ObjPtr<GC_Sound> _sound;
	float _timeFire;
	float _timeShot;
	bool _bFire;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Zippo : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Zippo);

public:
	GC_Weap_Zippo(World &world);
	GC_Weap_Zippo(FromFile);
	virtual ~GC_Weap_Zippo();

	// GC_Weapon
	virtual void AdjustVehicleClass(VehicleClass &vc) const;
	virtual void Fire(World &world, bool fire);
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);

	// GC_Pickup
	virtual void Attach(World &world, GC_Actor *actor);
	virtual void Detach(World &world);

	// GC_Object
	virtual void Kill(World &world);
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStep(World &world, float dt);
	
private:
	ObjPtr<GC_Sound> _sound;
	float _timeFire;
	float _timeShot;
	float _timeBurn;
	bool _bFire;
};

// end of file
