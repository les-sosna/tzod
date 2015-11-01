#pragma once
#include "Actor.h"
#include "ObjPtr.h"

class GC_RigidBodyStatic;
class GC_RigidBodyDynamic;
class GC_Light;
class GC_Vehicle;
class GC_Player;

#define GC_FLAG_PROJECTILE_ADVANCED      (GC_FLAG_ACTOR_ << 0)
#define GC_FLAG_PROJECTILE_TRAIL         (GC_FLAG_ACTOR_ << 1)
#define GC_FLAG_PROJECTILE_              (GC_FLAG_ACTOR_ << 2)

class GC_Projectile : public GC_Actor
{
    DECLARE_LIST_MEMBER(override);
    typedef GC_Actor base;

public:
	GC_Projectile(vec2d pos, vec2d v, GC_RigidBodyStatic *ignore, GC_Player *owner, bool advanced, bool trail);
	GC_Projectile(FromFile);
	virtual ~GC_Projectile();

	bool GetAdvanced() const { return CheckFlags(GC_FLAG_PROJECTILE_ADVANCED); }
	GC_RigidBodyStatic* GetIgnore() const { return _ignore; }
	GC_Player* GetOwner() const { return _owner; }
	float GetVelocity() const { return _velocity; }

	// GC_Actor
	void MoveTo(World &world, const vec2d &pos) override;

	// GC_Object
	void Init(World &world) override;
	void Kill(World &world) override;
	void Serialize(World &world, SaveFile &f) override;
	void TimeStep(World &world, float dt) override;
#ifdef NETWORK_DEBUG
	virtual DWORD checksum(void) const
	{
		DWORD cs = reinterpret_cast<const DWORD&>(GetPos().x);
		cs ^= reinterpret_cast<const DWORD&>(GetPos().y);
		cs ^= reinterpret_cast<const DWORD&>(_velocity);
		return GC_Actor::checksum() ^ cs;
	}
#endif

protected:
	float GetTrailDensity() { return _trailDensity; }
	void MoveWithTrail(World &world, const vec2d &pos, bool trail);
	void SetTrailDensity(float density);
	void SetVelocity(float v) { _velocity = v; }

	virtual bool OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth) = 0;
	virtual void SpawnTrailParticle(World &world, const vec2d &pos) = 0;

	// TODO: move to private
	ObjPtr<GC_Light>  _light;
private:
	float _velocity;
	ObjPtr<GC_RigidBodyStatic> _ignore;
	ObjPtr<GC_Player> _owner;
	float _trailDensity;
	float _trailPath;   // each time this value exceeds the _trailDensity, a trail particle is spawned
};

///////////////////////////////////////////////////////////////////////////////

class GC_Rocket : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_Rocket);

public:
	GC_Rocket(vec2d pos, vec2d v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_Rocket(FromFile);
	virtual ~GC_Rocket();

	void SelectTarget(World &world);

	// GC_Object
	void Serialize(World &world, SaveFile &f) override;
	void TimeStep(World &world, float dt) override;

protected:
	bool OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth) override;
	void SpawnTrailParticle(World &world, const vec2d &pos) override;

private:
	ObjPtr<GC_RigidBodyDynamic> _target;
	float _timeHomming;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Bullet : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_Bullet);

public:
	GC_Bullet(vec2d pos, vec2d v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_Bullet(FromFile);

	// GC_Object
	void Init(World &world) override;
	void Serialize(World &world, SaveFile &f) override;

protected:
	bool OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth) override;
	void SpawnTrailParticle(World &world, const vec2d &pos) override;

private:
	bool _trailEnable;
};

///////////////////////////////////////////////////////////////////////////////

class GC_TankBullet : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_TankBullet);

public:
	GC_TankBullet(vec2d pos, vec2d v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_TankBullet(FromFile);

	// GC_Object
	void Init(World &world) override;

protected:
	bool OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth) override;
	void SpawnTrailParticle(World &world, const vec2d &pos) override;
};

///////////////////////////////////////////////////////////////////////////////

class GC_PlazmaClod : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_PlazmaClod);

public:
	GC_PlazmaClod(vec2d pos, vec2d v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_PlazmaClod(FromFile);

protected:
	bool OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth) override;
	void SpawnTrailParticle(World &world, const vec2d &pos) override;
};

///////////////////////////////////////////////////////////////////////////////

class GC_BfgCore : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_BfgCore);

public:
	GC_BfgCore(vec2d pos, vec2d v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_BfgCore(FromFile);
	virtual ~GC_BfgCore();

	// GC_Object
    void Init(World &world) override;
    void Serialize(World &world, SaveFile &f) override;
    void TimeStep(World &world, float dt) override;

protected:
    bool OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth) override;
    void SpawnTrailParticle(World &world, const vec2d &pos) override;

private:
	ObjPtr<GC_RigidBodyDynamic> _target;
	GC_RigidBodyDynamic* FindTarget(World &world) const;
};

///////////////////////////////////////////////////////////////////////////////

#define GC_FLAG_FIRESPARK_SETFIRE      (GC_FLAG_PROJECTILE_ << 0)
#define GC_FLAG_FIRESPARK_HEALOWNER    (GC_FLAG_PROJECTILE_ << 1)

class GC_FireSpark : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_FireSpark);

public:
	GC_FireSpark(vec2d pos, vec2d v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_FireSpark(FromFile);

	float GetRadius() const { return (_time + 0.2f) * 50; }
	void SetHealOwner(bool heal);
	void SetLifeTime(float t);
	void SetSetFire(bool setFire) { SetFlags(GC_FLAG_FIRESPARK_SETFIRE, setFire); }

	// GC_Object
	void Init(World &world) override;
	void Serialize(World &world, SaveFile &f) override;
	void TimeStep(World &world, float dt) override;

protected:
	bool OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth) override;
	void SpawnTrailParticle(World &world, const vec2d &pos) override;

private:
	float _time;
	float _timeLife;
	float _rotation;
};

///////////////////////////////////////////////////////////////////////////////

class GC_ACBullet : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_ACBullet);

public:
	GC_ACBullet(vec2d pos, vec2d v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_ACBullet(FromFile);

	// GC_Object
	void Init(World &world) override;

protected:
	bool OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth) override;
	void SpawnTrailParticle(World &world, const vec2d &pos) override;
};

///////////////////////////////////////////////////////////////////////////////

class GC_GaussRay : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_GaussRay);

public:
	GC_GaussRay(vec2d pos, vec2d v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_GaussRay(FromFile);

	// GC_Object
	void Init(World &world) override;
	void Kill(World &world) override;
	void Serialize(World &world, SaveFile &f) override;

protected:
	bool OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth) override;
	void SpawnTrailParticle(World &world, const vec2d &pos) override;

private:
	float _damage;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Disk : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_Disk);

public:
	GC_Disk(vec2d pos, vec2d v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_Disk(FromFile);

	unsigned int GetBounces() const { return _bounces; }
	void SetBounces(unsigned int bounces) { _bounces = bounces; }

	// GC_Object
	void Init(World &world) override;
	void Serialize(World &world, SaveFile &f) override;

protected:
	bool OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth) override;
	void SpawnTrailParticle(World &world, const vec2d &pos) override;

private:
	unsigned int _bounces;
};
