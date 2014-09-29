// Turrets.h

#pragma once

#include "Rotator.h"
#include "RigidBody.h"

template<class T> class JobManager;
class GC_Vehicle;

class GC_Turret : public GC_RigidBodyStatic
{
    typedef GC_RigidBodyStatic base;
    
	class MyPropertySet : public GC_RigidBodyStatic::MyPropertySet
	{
		typedef GC_RigidBodyStatic::MyPropertySet BASE;
		ObjectProperty _propTeam;
		ObjectProperty _propSight;
        ObjectProperty _propDir;
	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void MyExchange(World &world, bool applyToObject);
	};
	virtual PropertySet* NewPropertySet();

protected:
	static JobManager<GC_Turret> _jobManager;

	ObjPtr<GC_Sound>         _rotateSound;
	ObjPtr<GC_Vehicle>       _target;

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
	virtual void CalcOutstrip(World &world, const GC_Vehicle *target, vec2d &fake) = 0;
	virtual void Fire(World &world) = 0;
	bool IsTargetVisible(World &world, GC_Vehicle* target, GC_RigidBodyStatic** pObstacle);
	virtual void TargetLost();
	GC_Vehicle* EnumTargets(World &world);
	void SelectTarget(World &world, GC_Vehicle *target);

	virtual void MapExchange(World &world, MapFile &f);

public:
    DECLARE_LIST_MEMBER();
	GC_Turret(World &world);
	GC_Turret(FromFile);
	virtual ~GC_Turret();
	
	float GetWeaponDir() const { return _dir; }
	virtual float GetReadyState() const { return 1; }
    
    virtual void SetInitialDir(float initialDir);

	virtual void MoveTo(World &world, const vec2d &pos);
	virtual void OnDestroy(World &world);

	// GC_Object
    virtual void Kill(World &world);
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStepFixed(World &world, float dt);
};

/////////////////////////////////////////////////////////////

class GC_TurretRocket : public GC_Turret
{
	DECLARE_SELF_REGISTRATION(GC_TurretRocket);

private:
	float _timeReload;

public:
	GC_TurretRocket(World &world);
	GC_TurretRocket(FromFile);
	virtual ~GC_TurretRocket();

	virtual float GetDefaultHealth() const { return 500; }
    
	virtual void Serialize(World &world, SaveFile &f);

	virtual unsigned char GetPassability() const { return 1; }

	virtual void CalcOutstrip(World &world, const GC_Vehicle *target, vec2d &fake);
	virtual void Fire(World &world);

	virtual void TimeStepFixed(World &world, float dt);
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
	GC_TurretCannon(World &world);
	GC_TurretCannon(FromFile);
	~GC_TurretCannon();

	virtual float GetDefaultHealth() const { return 600; }
    
	virtual void Serialize(World &world, SaveFile &f);

	virtual unsigned char GetPassability() const { return 1; }

	virtual void CalcOutstrip(World &world, const GC_Vehicle *target, vec2d &fake);
	virtual void Fire(World &world);

	virtual void TimeStepFixed(World &world, float dt);
};

/////////////////////////////////////////////////////////////

class GC_TurretBunker : public GC_Turret
{
private:
	float _time;

protected:
	void WakeUp(World &world);
	void WakeDown();

public:
	float _delta_angle;  // shooting accuracy control

public:
	float _time_wait_max;
	float _time_wait;

	float _time_wake;    // 0 - hidden
	float _time_wake_max;

public:
	GC_TurretBunker(World &world);
	GC_TurretBunker(FromFile);
	virtual ~GC_TurretBunker();
	
	virtual float GetReadyState() const { return _time_wake / _time_wake_max; }
    
    virtual void SetInitialDir(float initialDir);

	virtual void Serialize(World &world, SaveFile &f);

	virtual bool TakeDamage(World &world, float damage, const vec2d &hit, GC_Player *from);

	virtual void TimeStepFixed(World &world, float dt);
	virtual void MapExchange(World &world, MapFile &f);
};

/////////////////////////////////////////////////////////////

class GC_TurretMinigun : public GC_TurretBunker
{
	DECLARE_SELF_REGISTRATION(GC_TurretMinigun);

private:
	ObjPtr<GC_Sound> _fireSound;
	float _time;
	bool  _firing;

public:
	GC_TurretMinigun(World &world);
	GC_TurretMinigun(FromFile);
	virtual ~GC_TurretMinigun();

	virtual float GetDefaultHealth() const { return 250; }
    
    virtual void Kill(World &world);
	virtual void Serialize(World &world, SaveFile &f);

	virtual unsigned char GetPassability() const { return 1; }

	virtual void CalcOutstrip(World &world, const GC_Vehicle *target, vec2d &fake);
	virtual void Fire(World &world);

	virtual void TimeStepFixed(World &world, float dt);
};

/////////////////////////////////////////////////////////////

class GC_TurretGauss : public GC_TurretBunker
{
	DECLARE_SELF_REGISTRATION(GC_TurretGauss);

private:
	float   _time;
	int     _shotCount;

public:
	GC_TurretGauss(World &world);
	GC_TurretGauss(FromFile);
	virtual ~GC_TurretGauss();

	virtual void TargetLost();

	virtual unsigned char GetPassability() const { return 1; }

	virtual float GetDefaultHealth() const { return 250; }
    
	virtual void Serialize(World &world, SaveFile &f);

	virtual void CalcOutstrip(World &world, const GC_Vehicle *target, vec2d &fake);
	virtual void Fire(World &world);

	virtual void TimeStepFixed(World &world, float dt);
};

///////////////////////////////////////////////////////////////////////////////
// end of file
