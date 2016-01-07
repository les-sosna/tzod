#pragma once

#include "Pickup.h"
#include <cfloat>

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

#define GC_FLAG_WEAPON_FIRING            (GC_FLAG_PICKUP_ << 0)
#define GC_FLAG_WEAPON_                  (GC_FLAG_PICKUP_ << 1)

class GC_Weapon : public GC_Pickup
{
public:
	explicit GC_Weapon(vec2d pos);
	explicit GC_Weapon(FromFile);
	virtual ~GC_Weapon();

	GC_pu_Booster* GetBooster() const { return _booster; }
	float GetDetachedTime() const { return _detachedTime; }
	float GetStayTimeout() const { return _stayTimeout; }
	GC_Vehicle* GetVehicle() const { return _vehicle; }
	RotatorState GetRotationState() const { return _rotatorWeap.GetState(); }
	float GetRotationRate() const { return _rotatorWeap.GetVelocity() / _rotatorWeap.GetMaxVelocity(); }

	virtual void Fire(World &world, bool fire);
	virtual bool GetFire() const { return CheckFlags(GC_FLAG_WEAPON_FIRING); }
	virtual void SetBooster(World &world, GC_pu_Booster *booster) { _booster = booster; }
	virtual void SetupAI(AIWEAPSETTINGS *pSettings) = 0;
	virtual void AdjustVehicleClass(VehicleClass &vc) const = 0;

	// GC_Pickup
    void Detach(World &world) override;
    void Disappear(World &world) override;
    float GetDefaultRespawnTime() const override { return 6.0f; }
    AIPRIORITY GetPriority(World &world, const GC_Vehicle &veh) const override;

	// GC_Actor
    void MoveTo(World &world, const vec2d &pos) override;

	// GC_Object
    void Kill(World &world) override;
    void Serialize(World &world, SaveFile &f) override;
    void TimeStep(World &world, float dt) override;
#ifdef NETWORK_DEBUG
/*	virtual DWORD checksum(void) const
	{
		DWORD cs = reinterpret_cast<const DWORD&>(_angleReal)
			^ reinterpret_cast<const DWORD&>(_timeReload)
			^ reinterpret_cast<const DWORD&>(_timeStay);
		return GC_Pickup::checksum() ^ cs;
	}*/
#endif

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
    PropertySet* NewPropertySet() override;
    void OnAttached(World &world, GC_Vehicle &vehicle) override;

private:
	float _detachedTime = -FLT_MAX;
	float _stayTimeout;
	float _angle;
	Rotator _rotatorWeap;
	ObjPtr<GC_pu_Booster> _booster;
	ObjPtr<GC_Vehicle> _vehicle;

	virtual void OnUpdateView(World &world) {};
	void ProcessRotate(World &world, float dt);
};

/////////////////////////////////////////////////////////////////////////

#define GC_FLAG_PROJECTILEBASEDWEAPON_         (GC_FLAG_WEAPON_ << 0)

class ResumableObject;

class GC_ProjectileBasedWeapon : public GC_Weapon
{
public:
	explicit GC_ProjectileBasedWeapon(vec2d pos);
	explicit GC_ProjectileBasedWeapon(FromFile);
	virtual ~GC_ProjectileBasedWeapon();

	vec2d GetLastShotPos() const { return _lastShotPos; }
	float GetLastShotTime() const { return _lastShotTime; }
	unsigned int GetNumShots() const { return _numShots; }
	void Shoot(World &world);

	virtual bool GetContinuousSeries() const { return false; }
	virtual float GetFireEffectTime() const = 0;
	virtual float GetReloadTime() const = 0;
	virtual unsigned int GetSeriesLength() const { return 1; }
	virtual float GetSeriesReloadTime() const { return 0; }

	// GC_Weapon
    void Fire(World &world, bool fire) override;

	// GC_Pickup
    void Detach(World &world) override;

	// GC_Object
    void Resume(World &world) override;
    void Serialize(World &world, SaveFile &f) override;

protected:
	void SetLastShotPos(vec2d lastShotPos) { _lastShotPos = lastShotPos; }
	void ResetSeries();

    void OnAttached(World &world, GC_Vehicle &vehicle) override;

private:
	ObjPtr<GC_Light> _fireLight;
	float _lastShotTime;
	vec2d _lastShotPos;
	unsigned int _numShots;
	ResumableObject *_firing;

    void OnUpdateView(World &world) override;
	virtual void OnShoot(World &world) = 0;
};
