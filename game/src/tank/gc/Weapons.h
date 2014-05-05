// Weapons.h

#pragma once

#include "Pickup.h"

///////////////////////////////////////////////////////////////////////////////

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
	ObjPtr<GC_2dSprite> _fireEffect;
	ObjPtr<GC_Light>    _fireLight;
	vec2d _fePos;
	vec2d _feOrient;
	float _feTime;
	bool _advanced; // weapon has booster attached

public:
	virtual void SetAdvanced(World &world, bool advanced) { _advanced = advanced; }
	inline  bool GetAdvanced()              { return _advanced;     }

	GC_RigidBodyStatic* GetCarrier() const { return reinterpret_cast<GC_RigidBodyStatic *>(GC_Pickup::GetCarrier()); }

public:
	float _time;
	float _timeStay;
	float _timeReload;

	float    _angle;
	Rotator  _rotatorWeap;

	ObjPtr<GC_Sound>     _rotateSound;
	ObjPtr<GC_2dSprite>  _crosshair;
	bool _fixmeChAnimate;

public:
	GC_Weapon(World &world, float x, float y);
	GC_Weapon(FromFile);
	virtual ~GC_Weapon();
	virtual void Kill(World &world);

	virtual void Serialize(World &world, SaveFile &f);

	virtual float GetDefaultRespawnTime() const { return 6.0f; }
	virtual AIPRIORITY GetPriority(const GC_Vehicle &veh) const;
	virtual void SetupAI(AIWEAPSETTINGS *pSettings) = 0;

	virtual void Attach(World &world, GC_Actor *actor);
	virtual void Detach(World &world);
	void ProcessRotate(World &world, float dt);

	virtual void SetCrosshair(World &world);

	virtual void Fire(World &world) = 0;

	virtual void TimeStepFixed(World &world, float dt);
	virtual void TimeStepFloat(World &world, float dt);

private:
	virtual void OnUpdateView(World &world) {};

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
	float _time_shot;
	int _nshots_total;
	int _nshots;
	bool _firing;
	bool _reloaded;

	virtual void Attach(World &world, GC_Actor *actor);
	virtual void Detach(World &world);

	GC_Weap_RocketLauncher(World &world, float x, float y);
	GC_Weap_RocketLauncher(FromFile);

	virtual void Serialize(World &world, SaveFile &f);
	virtual void Fire(World &world);
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(World &world, float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_AutoCannon : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_AutoCannon);

public:
	virtual void SetAdvanced(World &world, bool advanced);

public:
	float _time_shot;
	int _nshots_total;
	int _nshots;
	bool _firing;

	virtual void Attach(World &world, GC_Actor *actor);
	virtual void Detach(World &world);

	GC_Weap_AutoCannon(World &world, float x, float y);
	GC_Weap_AutoCannon(FromFile);
	virtual ~GC_Weap_AutoCannon();

	virtual void Serialize(World &world, SaveFile &f);
	virtual void Fire(World &world);
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(World &world, float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Cannon : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Cannon);

private:
	float _time_smoke;
	float _time_smoke_dt;

public:
	virtual void Attach(World &world, GC_Actor *actor);

	GC_Weap_Cannon(World &world, float x, float y);
	GC_Weap_Cannon(FromFile);
	virtual ~GC_Weap_Cannon();

	virtual void Serialize(World &world, SaveFile &f);
	virtual void Fire(World &world);
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(World &world, float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Plazma : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Plazma);

public:
	virtual void Attach(World &world, GC_Actor *actor);

	GC_Weap_Plazma(World &world, float x, float y);
	GC_Weap_Plazma(FromFile);
	virtual ~GC_Weap_Plazma();

	virtual void Fire(World &world);
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Gauss : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Gauss);

public:
	virtual void Attach(World &world, GC_Actor *actor);

	GC_Weap_Gauss(World &world, float x, float y);
	GC_Weap_Gauss(FromFile);
	virtual ~GC_Weap_Gauss();

	virtual void Fire(World &world);
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Ram : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Ram);

private:
	ObjPtr<GC_Sound> _engineSound;
	ObjPtr<GC_Light> _engineLight;

protected:
	virtual void OnUpdateView(World &world);

public:
	float _fuel;
	float _fuel_max;
	float _fuel_consumption_rate;
	float _fuel_recuperation_rate;
	int _firingCounter;
	bool _bReady;

public:
	virtual void SetAdvanced(World &world, bool advanced);

public:
	virtual void Attach(World &world, GC_Actor *actor);
	virtual void Detach(World &world);

	GC_Weap_Ram(World &world, float x, float y);
	GC_Weap_Ram(FromFile);
	virtual ~GC_Weap_Ram();

    virtual void Kill(World &world);
	virtual void Serialize(World &world, SaveFile &f);
	virtual void Fire(World &world);
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(World &world, float dt);
	virtual void TimeStepFloat(World &world, float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_BFG : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_BFG);

private:
	float _time_ready;

public:
	virtual void Attach(World &world, GC_Actor *actor);

	GC_Weap_BFG(World &world, float x, float y);
	GC_Weap_BFG(FromFile);
	virtual ~GC_Weap_BFG();

	virtual void Serialize(World &world, SaveFile &f);
	virtual void Fire(World &world);
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(World &world, float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Ripper : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Ripper);

	ObjPtr<GC_2dSprite> _diskSprite;
	void UpdateDisk(World &world);

public:
	virtual void Attach(World &world, GC_Actor *actor);
	virtual void Detach(World &world);

	GC_Weap_Ripper(World &world, float x, float y);
	GC_Weap_Ripper(FromFile);
	virtual ~GC_Weap_Ripper();
	virtual void Serialize(World &world, SaveFile &f);

	virtual void Fire(World &world);
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFloat(World &world, float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Minigun : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Minigun);

private:
	ObjPtr<GC_Sound> _sound;
	float _timeRotate; // for firing animation
	float _timeFire;
	float _timeShot;
	bool _bFire;

	ObjPtr<GC_2dSprite> _crosshairLeft;

public:
	virtual void Attach(World &world, GC_Actor *actor);
	virtual void Detach(World &world);

	GC_Weap_Minigun(World &world, float x, float y);
	GC_Weap_Minigun(FromFile);
	virtual ~GC_Weap_Minigun();

	virtual void SetCrosshair(World &world);
    virtual void Kill(World &world);
	virtual void Serialize(World &world, SaveFile &f);
	virtual void Fire(World &world);
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(World &world, float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Zippo : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Zippo);

private:
	ObjPtr<GC_Sound> _sound;
	float _timeFire;
	float _timeShot;
	float _timeBurn;
	bool _bFire;

public:
	virtual void Attach(World &world, GC_Actor *actor);
	virtual void Detach(World &world);

	GC_Weap_Zippo(World &world, float x, float y);
	GC_Weap_Zippo(FromFile);
	virtual ~GC_Weap_Zippo();

    virtual void Kill(World &world);
	virtual void Serialize(World &world, SaveFile &f);
	virtual void Fire(World &world);
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(World &world, float dt);
};

///////////////////////////////////////////////////////////////////////////////
// end of file
