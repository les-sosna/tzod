// Turrets.h

#pragma once

#include "core/Rotator.h"
#include "RigidBody.h"

///////////////////////////////////////////////////////////////////////////////
// forward declarations

template<class T> class JobManager;
class GC_Vehicle;

///////////////////////////////////////////////////////////////////////////////

class GC_Turret : public GC_RigidBodyStatic
{
protected:
	static JobManager<GC_Turret> _jobManager;

	class MyPropertySet : public IPropertySet
	{
		typedef IPropertySet BASE;

		ObjectProperty _prop_team;
		ObjectProperty _prop_health;
		ObjectProperty _prop_health_max;
		ObjectProperty _prop_sight;

	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void Exchange(bool bApply);
	};

protected:
	SafePtr<GC_Sound>       _rotateSound;
	SafePtr<GC_Vehicle>     _target;
	SafePtr<GC_UserSprite>  _weaponSprite;

	enum enumTuretState
	{
		TS_ATACKING,
		TS_WAITING,
		TS_HIDDEN,
		TS_WAKING_UP,
		TS_WAKING_DOWN,
		TS_PREPARE_TO_WAKEDOWN
	};

public:
	enumTuretState	_state;

	int      _team;  // 0 - no team
	float    _initialDir;
	float    _sight;

	float    _dir;
	Rotator  _rotator;

protected:
	virtual void CalcOutstrip(const GC_Vehicle *target, vec2d &fake) = 0;

public:
	GC_Turret(float x, float y);
	GC_Turret(FromFile);

	virtual void Kill();
	virtual bool IsSaved() { return true; };
	virtual void Serialize(SaveFile &f);

	GC_Vehicle* EnumTargets();
	void SelectTarget(GC_Vehicle* target);
	virtual void TargetLost();

	bool IsTargetVisible(GC_Vehicle* target, GC_RigidBodyStatic** pObstacle);

	virtual void Fire() = 0;

	virtual void MoveTo(const vec2d &pos);
	virtual void OnDestroy();
	virtual bool TakeDamage(float damage, const vec2d &hit, GC_RigidBodyStatic *from);

	virtual void TimeStepFixed(float dt);
	virtual void Draw();

	////////////////////////////////////////////
	// editor functions
	virtual void EditorAction();
	virtual void mapExchange(MapFile &f);
	virtual SafePtr<IPropertySet> GetProperties();
};

/////////////////////////////////////////////////////////////

class GC_Turret_Rocket : public GC_Turret
{
	DECLARE_SELF_REGISTRATION(GC_Turret_Rocket);

private:
	float _time_reload;

public:
	GC_Turret_Rocket(float x, float y);
	GC_Turret_Rocket(FromFile);

	virtual float GetDefaultHealth() const { return 500; }
	virtual void Serialize(SaveFile &f);

	virtual unsigned char GetProperties() const { return 1; }

	virtual void CalcOutstrip(const GC_Vehicle *target, vec2d &fake);
	virtual void Fire();

	virtual void TimeStepFixed(float dt);
};

/////////////////////////////////////////////////////////////

class GC_Turret_Cannon : public GC_Turret
{
	DECLARE_SELF_REGISTRATION(GC_Turret_Cannon);

private:
	float _time_reload;
	float _time_smoke;
	float _time_smoke_dt;

public:
	GC_Turret_Cannon(float x, float y);
	GC_Turret_Cannon(FromFile);
	~GC_Turret_Cannon();

	virtual float GetDefaultHealth() const { return 600; }
	virtual void Serialize(SaveFile &f);

	virtual unsigned char GetProperties() const { return 1; }

	virtual void CalcOutstrip(const GC_Vehicle *target, vec2d &fake);
	virtual void Fire();

	virtual void TimeStepFixed(float dt);
};

/////////////////////////////////////////////////////////////

class GC_Turret_Bunker : public GC_Turret
{
private:
	float	_time;

protected:
	virtual void SetNormal(); // выбор обычной текстуры
	virtual void SetWaking(); // выбор "спрятаной" текстуры

	void WakeUp();
	void WakeDown();

public:
	float	_delta_angle;  // точность стрельбы

public:
	float	_time_wait;
	float	_time_wait_max;

	float	_time_wake;    // 0 - hidden
	float	_time_wake_max;

public:
	GC_Turret_Bunker(float x, float y);
	GC_Turret_Bunker(FromFile);
	virtual ~GC_Turret_Bunker();

	virtual void Serialize(SaveFile &f);

	virtual bool TakeDamage(float damage, const vec2d &hit, GC_RigidBodyStatic *from);

	virtual void TimeStepFixed(float dt);
	virtual void EditorAction();
	virtual void mapExchange(MapFile &f);
};

/////////////////////////////////////////////////////////////

class GC_Turret_Minigun : public GC_Turret_Bunker
{
	DECLARE_SELF_REGISTRATION(GC_Turret_Minigun);

private:
	SafePtr<GC_Sound> _fireSound;
	float	_time;
	bool	_firing;

public:
	GC_Turret_Minigun(float x, float y);
	GC_Turret_Minigun(FromFile);
	virtual ~GC_Turret_Minigun();
	virtual void Kill();

	virtual float GetDefaultHealth() const { return 250; }
	virtual void Serialize(SaveFile &f);

	virtual unsigned char GetProperties() const { return 1; }

	virtual void CalcOutstrip(const GC_Vehicle *target, vec2d &fake);
	virtual void Fire();

	virtual void TimeStepFixed(float dt);
};

/////////////////////////////////////////////////////////////

class GC_Turret_Gauss : public GC_Turret_Bunker
{
	DECLARE_SELF_REGISTRATION(GC_Turret_Gauss);

private:
	float	_time;
	int		_shot_count;

protected:
	virtual void SetNormal(); // выбор обычной текстуры
	virtual void SetWaking(); // выбор "спрятаной" текстуры

public:
	GC_Turret_Gauss(float x, float y);
	GC_Turret_Gauss(FromFile);
	virtual ~GC_Turret_Gauss();

	virtual void TargetLost();

	virtual unsigned char GetProperties() const { return 1; }

	virtual float GetDefaultHealth() const { return 250; }
	virtual void Serialize(SaveFile &f);

	virtual void CalcOutstrip(const GC_Vehicle *target, vec2d &fake);
	virtual void Fire();

	virtual void TimeStepFixed(float dt);
};


// end of file
