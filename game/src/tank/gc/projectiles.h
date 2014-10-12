// projectiles.h

#pragma once

#include "Actor.h"

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
    typedef GC_Actor base;

public:
    DECLARE_LIST_MEMBER();
	GC_Projectile(World &world, GC_RigidBodyStatic *ignore, GC_Player *owner, bool advanced, bool trail, const vec2d &pos, const vec2d &v);
	GC_Projectile(FromFile);
	virtual ~GC_Projectile();

	bool GetAdvanced() const { return CheckFlags(GC_FLAG_PROJECTILE_ADVANCED); }
	float GetHitDamage() const { return _hitDamage; }
	GC_RigidBodyStatic* GetIgnore() const { return _ignore; }
	float GetHitImpulse() const { return _hitImpulse; }
	GC_Player* GetOwner() const { return _owner; }
	float GetVelocity() const { return _velocity; }
	void SetHitDamage(float damage);
	void SetHitImpulse(float impulse);

	// GC_Object
    virtual void Kill(World &world);
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStep(World &world, float dt);
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
	void ApplyHitDamage(World &world, GC_RigidBodyStatic *target, const vec2d &hitPoint);
	float GetTrailDensity() { return _trailDensity; }
	void MoveWithTrail(World &world, const vec2d &pos, bool trail);
	void SetTrailDensity(World &world, float density);
	void SetVelocity(float v) { _velocity = v; }
	
	virtual bool OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth) = 0;
	virtual void SpawnTrailParticle(World &world, const vec2d &pos) = 0;
	virtual float FilterDamage(float damage, GC_RigidBodyStatic *object);

	// TODO: move to private
	ObjPtr<GC_Light>  _light;
private:
	float _velocity;
	ObjPtr<GC_RigidBodyStatic> _ignore;
	ObjPtr<GC_Player> _owner;
	float _hitDamage;   // negative damage will heal target
	float _hitImpulse;
	float _trailDensity;
	float _trailPath;   // each time this value exceeds the _trailDensity, a trail particle is spawned
};

///////////////////////////////////////////////////////////////////////////////

class GC_Rocket : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_Rocket);

public:
	GC_Rocket(World &world, const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_Rocket(FromFile);
	virtual ~GC_Rocket();

	// GC_Object
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStep(World &world, float dt);
	
protected:
	virtual bool OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth) override;
	virtual void SpawnTrailParticle(World &world, const vec2d &pos) override;
	
private:
	ObjPtr<GC_RigidBodyDynamic> _target;
	float _timeHomming;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Bullet : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_Bullet);

public:
	GC_Bullet(World &world, const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_Bullet(FromFile);

	// GC_Object
	virtual void Serialize(World &world, SaveFile &f);
	
protected:
	virtual bool OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth) override;
	virtual void SpawnTrailParticle(World &world, const vec2d &pos) override;

private:
	bool _trailEnable;
};

///////////////////////////////////////////////////////////////////////////////

class GC_TankBullet : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_TankBullet);

public:
	GC_TankBullet(World &world, const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_TankBullet(FromFile);

protected:
	virtual bool OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth) override;
	virtual void SpawnTrailParticle(World &world, const vec2d &pos) override;
};

///////////////////////////////////////////////////////////////////////////////

class GC_PlazmaClod : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_PlazmaClod);

public:
	GC_PlazmaClod(World &world, const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_PlazmaClod(FromFile);

protected:
	virtual bool OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth) override;
	virtual void SpawnTrailParticle(World &world, const vec2d &pos) override;
};

///////////////////////////////////////////////////////////////////////////////

class GC_BfgCore : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_BfgCore);

public:
	GC_BfgCore(World &world, const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_BfgCore(FromFile);
	virtual ~GC_BfgCore();

	// GC_Object
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStep(World &world, float dt);

protected:
	virtual bool OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth) override;
	virtual void SpawnTrailParticle(World &world, const vec2d &pos) override;

private:
	ObjPtr<GC_RigidBodyDynamic> _target;
	float _time;
	void FindTarget(World &world);
};

///////////////////////////////////////////////////////////////////////////////

#define GC_FLAG_FIRESPARK_SETFIRE      (GC_FLAG_PROJECTILE_ << 0)
#define GC_FLAG_FIRESPARK_HEALOWNER    (GC_FLAG_PROJECTILE_ << 1)

class GC_FireSpark : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_FireSpark);

public:
	GC_FireSpark(World &world, const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_FireSpark(FromFile);
	
	float GetRadius() const { return (_time + 0.2f) * 50; }
	void SetHealOwner(bool heal);
	void SetLifeTime(float t);
	void SetSetFire(bool setFire) { SetFlags(GC_FLAG_FIRESPARK_SETFIRE, setFire); }

	// GC_Object
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStep(World &world, float dt);
	
protected:
	virtual bool OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth) override;
	virtual void SpawnTrailParticle(World &world, const vec2d &pos) override;
	virtual float FilterDamage(float damage, GC_RigidBodyStatic *object) override;

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
	GC_ACBullet(World &world, const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_ACBullet(FromFile);

protected:
	virtual bool OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth) override;
	virtual void SpawnTrailParticle(World &world, const vec2d &pos) override;
};

///////////////////////////////////////////////////////////////////////////////

class GC_GaussRay : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_GaussRay);

public:
	GC_GaussRay(World &world, const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_GaussRay(FromFile);
	
	// GC_Object
    virtual void Kill(World &world);
	
protected:
	virtual bool OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth) override;
	virtual void SpawnTrailParticle(World &world, const vec2d &pos) override;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Disk : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_Disk);

public:
	GC_Disk(World &world, const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_Disk(FromFile);

protected:
	virtual bool OnHit(World &world, GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth) override;
	virtual void SpawnTrailParticle(World &world, const vec2d &pos) override;
	virtual float FilterDamage(float damage, GC_RigidBodyStatic *object) override;
};
