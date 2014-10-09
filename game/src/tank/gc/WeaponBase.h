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
