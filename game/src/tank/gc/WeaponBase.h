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

#define GC_FLAG_WEAPON_ADVANCED          (GC_FLAG_PICKUP_ << 0)
#define GC_FLAG_WEAPON_FIRING            (GC_FLAG_PICKUP_ << 1)
#define GC_FLAG_WEAPON_                  (GC_FLAG_PICKUP_ << 2)

class GC_Weapon : public GC_Pickup
{
public:
	GC_Weapon(World &world);
	GC_Weapon(FromFile);
	virtual ~GC_Weapon();
	
	bool GetFire() const { return CheckFlags(GC_FLAG_WEAPON_FIRING); }
	float GetDetachedTime() const { return _detachedTime; }
	float GetStayTimeout() const { return _stayTimeout; }
	bool GetAdvanced() const { return CheckFlags(GC_FLAG_WEAPON_ADVANCED); }
	GC_RigidBodyStatic* GetCarrier() const { return reinterpret_cast<GC_RigidBodyStatic *>(GC_Pickup::GetCarrier()); }
	
	virtual void Fire(World &world, bool fire);
	virtual void SetAdvanced(World &world, bool advanced) { SetFlags(GC_FLAG_WEAPON_ADVANCED, advanced); }
	virtual void SetupAI(AIWEAPSETTINGS *pSettings) = 0;
	virtual void AdjustVehicleClass(VehicleClass &vc) const = 0;

	// GC_Pickup
	virtual void Attach(World &world, GC_Actor *actor) override;
	virtual void Detach(World &world) override;
	virtual float GetDefaultRespawnTime() const override { return 6.0f; }
	virtual AIPRIORITY GetPriority(World &world, const GC_Vehicle &veh) const override;
	
	// GC_Object
	virtual void Kill(World &world);
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStep(World &world, float dt);
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
	virtual PropertySet* NewPropertySet();

private:
	float _detachedTime = -FLT_MAX;
	float _stayTimeout;
	float _angle;
	Rotator _rotatorWeap;
	ObjPtr<GC_Sound> _rotateSound;
	
	virtual void OnUpdateView(World &world) {};
	void ProcessRotate(World &world, float dt);
};

/////////////////////////////////////////////////////////////////////////

#define GC_FLAG_PROJECTILEBASEDWEAPON_         (GC_FLAG_WEAPON_ << 0)

class ResumableObject;

class GC_ProjectileBasedWeapon : public GC_Weapon
{
public:
	GC_ProjectileBasedWeapon(World &world);
	GC_ProjectileBasedWeapon(FromFile);
	virtual ~GC_ProjectileBasedWeapon();

	vec2d GetLastShotPos() const { return _lastShotPos; }
	float GetLastShotTime() const { return _lastShotTime; }
	unsigned int GetNumShots() const { return _numShots; }
	float GetStartTime() const { return _startTime; }
	float GetStopTime() const { return _stopTime; }

	virtual bool GetContinuousSeries() const { return false; }
	virtual float GetFireEffectTime() const = 0;
	virtual float GetReloadTime() const = 0;
	virtual unsigned int GetSeriesLength() const { return 1; }
	virtual float GetSeriesReloadTime() const { return 0; }

	// GC_Weapon
	virtual void Fire(World &world, bool fire);

	// GC_Pickup
	virtual void Attach(World &world, GC_Actor *actor) override;
	virtual void Detach(World &world) override;

	// GC_Object
	virtual void Resume(World &world) override;
	virtual void Serialize(World &world, SaveFile &f) override;

protected:
	void SetLastShotPos(vec2d lastShotPos) { _lastShotPos = lastShotPos; }

	// TODO: make private
	ObjPtr<GC_Light> _fireLight;
	float _lastShotTime;
private:
	vec2d _lastShotPos;
	unsigned int _numShots;
	float _startTime;
	float _stopTime;
	ResumableObject *_firing;

	virtual void OnUpdateView(World &world) override;
	virtual void OnShoot(World &world) = 0;
};
