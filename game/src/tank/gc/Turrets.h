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
	class MyPropertySet : public GC_RigidBodyStatic::MyPropertySet
	{
		typedef GC_RigidBodyStatic::MyPropertySet BASE;
		ObjectProperty _propTeam;
		ObjectProperty _propSight;
	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void MyExchange(bool applyToObject);
	};
	virtual PropertySet* NewPropertySet();

protected:
	static JobManager<GC_Turret> _jobManager;

	SafePtr<GC_Sound>       _rotateSound;
	SafePtr<GC_Vehicle>     _target;
	SafePtr<GC_2dSprite>    _weaponSprite;

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
	enumTuretState _state;

	int      _team;  // 0 - no team
	float    _initialDir;
	float    _sight;

	float    _dir; // linked with the rotator
	Rotator  _rotator;

protected:
	virtual void CalcOutstrip(const GC_Vehicle *target, vec2d &fake) = 0;
	virtual void Fire() = 0;
	bool IsTargetVisible(GC_Vehicle* target, GC_RigidBodyStatic** pObstacle);
	virtual void TargetLost();
	GC_Vehicle* EnumTargets();
	void SelectTarget(const SafePtr<GC_Vehicle> &target);

	// editor functions
	virtual void EditorAction();
	virtual void MapExchange(MapFile &f);

public:
	GC_Turret(float x, float y, const char *tex);
	GC_Turret(FromFile);
	virtual ~GC_Turret();

	virtual void Kill();
	virtual void Serialize(SaveFile &f);


	virtual void MoveTo(const vec2d &pos);
	virtual void OnDestroy();

	virtual void TimeStepFixed(float dt);
	virtual void Draw() const;
};

/////////////////////////////////////////////////////////////

class GC_TurretRocket : public GC_Turret
{
	DECLARE_SELF_REGISTRATION(GC_TurretRocket);

private:
	float _timeReload;

public:
	GC_TurretRocket(float x, float y);
	GC_TurretRocket(FromFile);

	virtual float GetDefaultHealth() const { return 500; }
	virtual void Serialize(SaveFile &f);

	virtual unsigned char GetPassability() const { return 1; }

	virtual void CalcOutstrip(const GC_Vehicle *target, vec2d &fake);
	virtual void Fire();

	virtual void TimeStepFixed(float dt);
};

/////////////////////////////////////////////////////////////

class GC_TurretCannon : public GC_Turret
{
	DECLARE_SELF_REGISTRATION(GC_TurretCannon);

private:
	float _timeReload;
	float _time_smoke;
	float _time_smoke_dt;

public:
	GC_TurretCannon(float x, float y);
	GC_TurretCannon(FromFile);
	~GC_TurretCannon();

	virtual float GetDefaultHealth() const { return 600; }
	virtual void Serialize(SaveFile &f);

	virtual unsigned char GetPassability() const { return 1; }

	virtual void CalcOutstrip(const GC_Vehicle *target, vec2d &fake);
	virtual void Fire();

	virtual void TimeStepFixed(float dt);
};

/////////////////////////////////////////////////////////////

class GC_TurretBunker : public GC_Turret
{
private:
	float _time;

protected:
	void WakeUp();
	void WakeDown();

public:
	float _delta_angle;  // shooting accuracy control

public:
	float _time_wait;
	float _time_wait_max;

	float _time_wake;    // 0 - hidden
	float _time_wake_max;

public:
	GC_TurretBunker(float x, float y, const char *tex);
	GC_TurretBunker(FromFile);
	virtual ~GC_TurretBunker();

	virtual void Serialize(SaveFile &f);

	virtual bool TakeDamage(float damage, const vec2d &hit, GC_Player *from);

	virtual void TimeStepFixed(float dt);
	virtual void EditorAction();
	virtual void MapExchange(MapFile &f);
};

/////////////////////////////////////////////////////////////

class GC_TurretMinigun : public GC_TurretBunker
{
	DECLARE_SELF_REGISTRATION(GC_TurretMinigun);

private:
	SafePtr<GC_Sound> _fireSound;
	float _time;
	bool  _firing;

public:
	GC_TurretMinigun(float x, float y);
	GC_TurretMinigun(FromFile);
	virtual ~GC_TurretMinigun();
	virtual void Kill();

	virtual float GetDefaultHealth() const { return 250; }
	virtual void Serialize(SaveFile &f);

	virtual unsigned char GetPassability() const { return 1; }

	virtual void CalcOutstrip(const GC_Vehicle *target, vec2d &fake);
	virtual void Fire();

	virtual void TimeStepFixed(float dt);
};

/////////////////////////////////////////////////////////////

class GC_TurretGauss : public GC_TurretBunker
{
	DECLARE_SELF_REGISTRATION(GC_TurretGauss);

private:
	float   _time;
	int     _shotCount;

public:
	GC_TurretGauss(float x, float y);
	GC_TurretGauss(FromFile);
	virtual ~GC_TurretGauss();

	virtual void TargetLost();

	virtual unsigned char GetPassability() const { return 1; }

	virtual float GetDefaultHealth() const { return 250; }
	virtual void Serialize(SaveFile &f);

	virtual void CalcOutstrip(const GC_Vehicle *target, vec2d &fake);
	virtual void Fire();

	virtual void TimeStepFixed(float dt);
};

///////////////////////////////////////////////////////////////////////////////
// end of file
