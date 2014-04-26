// projectiles.h

#pragma once

#include "2dSprite.h"

///////////////////////////////////////////////////////////////////////////////
// forward declarations

class GC_RigidBodyStatic;
class GC_RigidBodyDynamic;
class GC_Light;
class GC_Vehicle;
class GC_Player;

///////////////////////////////////////////////////////////////////////////////

#define GC_FLAG_PROJECTILE_ADVANCED      (GC_FLAG_2DSPRITE_ << 0)
#define GC_FLAG_PROJECTILE_TRAIL         (GC_FLAG_2DSPRITE_ << 1)
#define GC_FLAG_PROJECTILE_              (GC_FLAG_2DSPRITE_ << 2)

class GC_Projectile : public GC_2dSprite
{
	ObjPtr<GC_RigidBodyStatic> _ignore;
	ObjPtr<GC_Player> _owner;

protected:
	ObjPtr<GC_Light>  _light;

	float _velocity;
	inline GC_RigidBodyStatic* GetIgnore() const;
	GC_Player* GetOwner() const { return _owner; }

private:

	float _hitDamage;   // negative damage will heal target
	float _hitImpulse;

	float _trailDensity;
	float _trailPath;   // each time this value exceeds the _trailDensity, a trail particle is spawned

protected:
	virtual void MoveTo(const vec2d &pos, bool trail);
	virtual bool OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth) = 0;
	virtual void SpawnTrailParticle(const vec2d &pos) = 0;
	virtual float FilterDamage(float damage, GC_RigidBodyStatic *object);

	void SetTrailDensity(float density);
	float GetTrailDensity() { return _trailDensity; }

	void ApplyHitDamage(GC_RigidBodyStatic *target, const vec2d &hitPoint);

public:
	GC_Projectile(GC_RigidBodyStatic *ignore, GC_Player *owner, bool advanced,
		bool trail, const vec2d &pos, const vec2d &v, const char *texture);
	GC_Projectile(FromFile);
	virtual ~GC_Projectile();

	void SetHitDamage(float damage);
	float GetHitDamage() const { return _hitDamage; }

	void SetHitImpulse(float impulse);
	float GetHitImpulse() const { return _hitImpulse; }

	bool GetAdvanced() const
	{
		return CheckFlags(GC_FLAG_PROJECTILE_ADVANCED);
	}

	virtual void Serialize(SaveFile &f);
	virtual void TimeStepFixed(float dt);

#ifdef NETWORK_DEBUG
public:
	virtual DWORD checksum(void) const
	{
		DWORD cs = reinterpret_cast<const DWORD&>(GetPos().x);
		cs ^= reinterpret_cast<const DWORD&>(GetPos().y);
		cs ^= reinterpret_cast<const DWORD&>(_velocity);
		return GC_2dSprite::checksum() ^ cs;
	}
#endif
};

///////////////////////////////////////////////////////////////////////////////

class GC_Rocket : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_Rocket);

private:
	ObjPtr<GC_RigidBodyDynamic> _target;
	float _timeHomming;

public:
	GC_Rocket(const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_Rocket(FromFile);
	virtual ~GC_Rocket();

	virtual void Serialize(SaveFile &f);

	virtual bool OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth);
	virtual void SpawnTrailParticle(const vec2d &pos);

	virtual void TimeStepFixed(float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Bullet : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_Bullet);

private:
	bool _trailEnable;

public:
	GC_Bullet(const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_Bullet(FromFile);
	virtual ~GC_Bullet();

	virtual void Serialize(SaveFile &f);
	virtual bool OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth);
	virtual void SpawnTrailParticle(const vec2d &pos);
};

///////////////////////////////////////////////////////////////////////////////

class GC_TankBullet : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_TankBullet);

public:
	GC_TankBullet(const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_TankBullet(FromFile);
	virtual ~GC_TankBullet();

	virtual bool OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth);
	virtual void SpawnTrailParticle(const vec2d &pos);
};

///////////////////////////////////////////////////////////////////////////////

class GC_PlazmaClod : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_PlazmaClod);

public:
	GC_PlazmaClod(const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_PlazmaClod(FromFile);
	virtual ~GC_PlazmaClod();

	virtual bool OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth);
	virtual void SpawnTrailParticle(const vec2d &pos);
};

///////////////////////////////////////////////////////////////////////////////

class GC_BfgCore : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_BfgCore);
private:
	ObjPtr<GC_RigidBodyDynamic> _target;
	float _time;
	void FindTarget();

public:
	GC_BfgCore(const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_BfgCore(FromFile);
	virtual ~GC_BfgCore();

	virtual void Serialize(SaveFile &f);

	virtual bool OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth);
	virtual void SpawnTrailParticle(const vec2d &pos);

	virtual void TimeStepFixed(float dt);
};

///////////////////////////////////////////////////////////////////////////////

#define GC_FLAG_FIRESPARK_SETFIRE      (GC_FLAG_PROJECTILE_ << 0)
#define GC_FLAG_FIRESPARK_HEALOWNER    (GC_FLAG_PROJECTILE_ << 1)

class GC_FireSpark : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_FireSpark);

private:
	float _time;
	float _timeLife;
	float _rotation;

	float GetRadius() const { return (_time + 0.2f) * 50; }

public:
	GC_FireSpark(const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_FireSpark(FromFile);
	virtual ~GC_FireSpark();

	virtual void Serialize(SaveFile &f);
	virtual void Draw(bool editorMode) const;

	virtual bool OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth);
	virtual void SpawnTrailParticle(const vec2d &pos);
	virtual float FilterDamage(float damage, GC_RigidBodyStatic *object);

	virtual void TimeStepFixed(float dt);

	void SetHealOwner(bool heal);
	void SetLifeTime(float t);
	void SetSetFire(bool setFire)
	{
		SetFlags(GC_FLAG_FIRESPARK_SETFIRE, setFire);
	}
};

///////////////////////////////////////////////////////////////////////////////

class GC_ACBullet : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_ACBullet);

public:
	GC_ACBullet(const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_ACBullet(FromFile);
	virtual ~GC_ACBullet();

	virtual bool OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth);
	virtual void SpawnTrailParticle(const vec2d &pos);
};

///////////////////////////////////////////////////////////////////////////////

class GC_GaussRay : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_GaussRay);

public:
	GC_GaussRay(const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_GaussRay(FromFile);
	virtual ~GC_GaussRay();

	virtual bool OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth);
	virtual void SpawnTrailParticle(const vec2d &pos);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Disk : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_Disk);

public:
	GC_Disk(const vec2d &x, const vec2d &v, GC_RigidBodyStatic *ignore, GC_Player* owner, bool advanced);
	GC_Disk(FromFile);
	virtual ~GC_Disk();

protected:
	virtual bool OnHit(GC_RigidBodyStatic *object, const vec2d &hit, const vec2d &norm, float relativeDepth);
	virtual void SpawnTrailParticle(const vec2d &pos);
	virtual float FilterDamage(float damage, GC_RigidBodyStatic *object);
};

///////////////////////////////////////////////////////////////////////////////
// end of file
