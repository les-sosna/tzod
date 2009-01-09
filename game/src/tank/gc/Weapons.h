// Weapons.h

#pragma once

#include "Pickup.h"

///////////////////////////////////////////////////////////////////////////////

struct AIWEAPSETTINGS
{
	float fMaxAttackAngle;     // максимальный прицельный угол
	float fProjectileSpeed;    // скорость снаряда
	float fAttackRadius_min;   // минимальный радиус атаки
	float fAttackRadius_max;   // максимальный радиус атаки
	float fAttackRadius_crit;  // критический радиус атаки, когда можно убиться
	float fDistanceMultipler;  // сложность пробивания стен
	bool  bNeedOutstrip;       // FALSE, если мгновенное оружие (gauss, ...)
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
		virtual void MyExchange(bool applyToObject);
	};
	virtual PropertySet* NewPropertySet();

protected:

	SafePtr<GC_UserSprite> _fireEffect;
	SafePtr<GC_Light>      _fireLight;
	vec2d _fePos;
	float _feTime;
	float _feOrient;
	bool _advanced; // weapon has booster attached

	virtual void UpdateView();

public:
	virtual void SetAdvanced(bool advanced) { _advanced = advanced; }
	inline  bool GetAdvanced()              { return _advanced;     }

	GC_RigidBodyStatic* GetOwner() const { return (GC_RigidBodyStatic *) GC_Pickup::GetOwner(); }

public:
	float _time;
	float _timeStay;
	float _timeReload;

	float    _angleReal;          // note that sprite rotation is predicted angle
	Rotator  _rotator;

	SafePtr<GC_Crosshair> _crosshair;
	SafePtr<GC_Sound>     _rotateSound;

public:
	GC_Weapon(float x, float y);
	GC_Weapon(FromFile);
	virtual ~GC_Weapon();
	virtual void Kill();

	virtual void Serialize(SaveFile &f);

	virtual float GetDefaultRespawnTime() const { return 6.0f; }
	virtual AIPRIORITY GetPriority(GC_Vehicle *veh);
	virtual void SetupAI(AIWEAPSETTINGS *pSettings) = 0;

	virtual void Attach(GC_Actor *actor);
	virtual void Detach();
	void ProcessRotate(float dt);

	virtual void SetCrosshair();

	virtual void Fire() = 0;

	virtual void TimeStepFixed(float dt);
	virtual void TimeStepFloat(float dt);
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

	virtual void Attach(GC_Actor *actor);
	virtual void Detach();

	GC_Weap_RocketLauncher(float x, float y);
	GC_Weap_RocketLauncher(FromFile);

	virtual void Serialize(SaveFile &f);
	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_AutoCannon : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_AutoCannon);

public:
	virtual void SetAdvanced(bool advanced);

public:
	float _time_shot;
	int _nshots_total;
	int _nshots;
	bool _firing;

	virtual void Attach(GC_Actor *actor);
	virtual void Detach();

	GC_Weap_AutoCannon(float x, float y);
	GC_Weap_AutoCannon(FromFile);
	virtual ~GC_Weap_AutoCannon();
	virtual void Kill();

	virtual void Serialize(SaveFile &f);
	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Cannon : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Cannon);

private:
	float _time_smoke;
	float _time_smoke_dt;

public:
	virtual void Attach(GC_Actor *actor);

	GC_Weap_Cannon(float x, float y);
	GC_Weap_Cannon(FromFile);
	virtual ~GC_Weap_Cannon();

	virtual void Serialize(SaveFile &f);
	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Plazma : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Plazma);

public:
	virtual void Attach(GC_Actor *actor);

	GC_Weap_Plazma(float x, float y);
	GC_Weap_Plazma(FromFile);
	virtual ~GC_Weap_Plazma();

	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Gauss : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Gauss);

public:
	virtual void Attach(GC_Actor *actor);

	GC_Weap_Gauss(float x, float y);
	GC_Weap_Gauss(FromFile);
	virtual ~GC_Weap_Gauss();

	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Ram : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Ram);

private:
	SafePtr<GC_Sound> _engineSound;
	SafePtr<GC_Light> _engineLight;

protected:
	virtual void UpdateView();

public:
	float _fuel;
	float _fuel_max;
	float _fuel_rate;  // расход топлива в сек.
	float _fuel_rep;   // восстановление топлива в сек.
	int _firingCounter;
	bool _bReady;

public:
	virtual void SetAdvanced(bool advanced);

public:
	virtual void Attach(GC_Actor *actor);
	virtual void Detach();

	GC_Weap_Ram(float x, float y);
	GC_Weap_Ram(FromFile);
	virtual ~GC_Weap_Ram();
	virtual void Kill();

	virtual void Serialize(SaveFile &f);
	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(float dt);
	virtual void TimeStepFloat(float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_BFG : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_BFG);

private:
	float _time_ready;

public:
	virtual void Attach(GC_Actor *actor);

	GC_Weap_BFG(float x, float y);
	GC_Weap_BFG(FromFile);
	virtual ~GC_Weap_BFG();

	virtual void Serialize(SaveFile &f);
	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Ripper : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Ripper);

	SafePtr<GC_UserSprite> _disk;
	void UpdateDisk();

public:
	virtual void Attach(GC_Actor *actor);
	virtual void Detach();

	GC_Weap_Ripper(float x, float y);
	GC_Weap_Ripper(FromFile);
	virtual ~GC_Weap_Ripper();
	virtual void Serialize(SaveFile &f);

	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFloat(float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Minigun : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Minigun);

private:
	SafePtr<GC_Sound> _sound;
	float _timeRotate; // для эмуляции вращения стволов
	float _timeFire;   // время непрерывнго огня
	float _timeShot;
	bool _bFire;

	SafePtr<GC_Crosshair> _crosshairLeft;

public:
	virtual void Attach(GC_Actor *actor);
	virtual void Detach();

	GC_Weap_Minigun(float x, float y);
	GC_Weap_Minigun(FromFile);
	virtual ~GC_Weap_Minigun();
	virtual void Kill();

	virtual void SetCrosshair();

	virtual void Serialize(SaveFile &f);
	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Weap_Zippo : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Zippo);

private:
	SafePtr<GC_Sound> _sound;
	float _timeFire;   // время непрерывнго огня
	float _timeShot;
	float _timeBurn;
	bool _bFire;

public:
	virtual void Attach(GC_Actor *actor);
	virtual void Detach();

	GC_Weap_Zippo(float x, float y);
	GC_Weap_Zippo(FromFile);
	virtual ~GC_Weap_Zippo();
	virtual void Kill();

	virtual void Serialize(SaveFile &f);
	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(float dt);
};

///////////////////////////////////////////////////////////////////////////////
// end of file
