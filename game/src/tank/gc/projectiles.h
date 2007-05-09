// projectiles.h

#pragma once

#include "2dSprite.h"

////////////////////////////////////////////////////////////
// forward declarations

class GC_RigidBodyStatic;
class GC_RigidBodyDynamic;
class GC_Light;
class GC_Vehicle;

////////////////////////////////////////////////////////////

#define GC_FLAG_PROJECTILE_ADVANCED           (GC_FLAG_2DSPRITE_ << 0)
#define GC_FLAG_PROJECTILE_TRAIL              (GC_FLAG_2DSPRITE_ << 1)
#define GC_FLAG_PROJECTILE_IGNOREPROPRIETOR   (GC_FLAG_2DSPRITE_ << 2)

class GC_Projectile : public GC_2dSprite
{
	MemberOfGlobalList _memberOf;

protected:
	SafePtr<GC_RigidBodyStatic> _proprietor;
	SafePtr<GC_RigidBodyStatic> _lastHit;
	SafePtr<GC_Light>           _light;

	float _trailDensity;
	float _trailPath;		// когда это значение превышает _trailDensity,
							// рождается TrailParticle

protected:
	virtual void MoveTo(const vec2d &pos, BOOL bTrail);
	virtual bool Hit(GC_Object *object, const vec2d &hit, const vec2d &norm) = 0;
	virtual void SpawnTrailParticle(const vec2d &pos) = 0;

	vec2d _velocity;

public:
	float _Damage;

	GC_Projectile(GC_RigidBodyStatic *pProprietor, bool advanced,
		bool bTrail, const vec2d &pos, const vec2d &v, const char *texture);
	GC_Projectile(FromFile);

	bool IsAdvanced() const
	{
		return CheckFlags(GC_FLAG_PROJECTILE_ADVANCED);
	}
	void SetIgnoreProprietor(bool bIgnore)
	{
		bIgnore ? SetFlags(GC_FLAG_PROJECTILE_IGNOREPROPRIETOR)
			: ClearFlags(GC_FLAG_PROJECTILE_IGNOREPROPRIETOR);
	}

	void SpecialTrace(GC_RigidBodyDynamic *pObj, const vec2d &path);

	virtual void Kill();
	virtual void Serialize(SaveFile &f);
	virtual bool IsSaved() { return true; };

	virtual void TimeStepFixed(float dt);

#ifdef NETWORK_DEBUG
public:
	virtual DWORD checksum(void) const
	{
		DWORD cs = reinterpret_cast<const DWORD&>(_pos.x);
		cs ^= reinterpret_cast<const DWORD&>(_pos.y);
		cs ^= reinterpret_cast<const DWORD&>(_velocity.x);
		cs ^= reinterpret_cast<const DWORD&>(_velocity.y);
		return GC_2dSprite::checksum() ^ cs;
	}
#endif
};

/////////////////////////////////////////////////////////////

class GC_Rocket : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_Rocket);

private:
	SafePtr<GC_Vehicle> _target;
	float _time;

protected:
	void FreeTarget();

public:
	GC_Rocket(const vec2d &x, const vec2d &v, GC_RigidBodyStatic* SpawnBy, bool advanced);
	GC_Rocket(FromFile);
	virtual ~GC_Rocket();

	virtual void Kill();

	virtual void Serialize(SaveFile &f);


	virtual bool Hit(GC_Object *object, const vec2d &hit, const vec2d &norm);
	virtual void SpawnTrailParticle(const vec2d &pos);

	virtual void TimeStepFixed(float dt);
};


/////////////////////////////////////////////////////////////


class GC_Bullet : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_Bullet);

private:
	float _path;
	float _path_trail_on;
	float _path_trail_off;

public:
	GC_Bullet(const vec2d &x, const vec2d &v, GC_RigidBodyStatic* SpawnBy, bool advanced);
	GC_Bullet(FromFile);

	virtual void Serialize(SaveFile &f);
	virtual bool Hit(GC_Object *object, const vec2d &hit, const vec2d &norm);
	virtual void SpawnTrailParticle(const vec2d &pos);
};

/////////////////////////////////////////////////////////////

class GC_TankBullet : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_TankBullet);

public:
	GC_TankBullet(const vec2d &x, const vec2d &v, GC_RigidBodyStatic* SpawnBy, bool advanced);
	GC_TankBullet(FromFile) : GC_Projectile(FromFile()) {}

	virtual bool Hit(GC_Object *object, const vec2d &hit, const vec2d &norm);
	virtual void SpawnTrailParticle(const vec2d &pos);
};

/////////////////////////////////////////////////////////////

class GC_PlazmaClod : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_PlazmaClod);

public:
	GC_PlazmaClod(const vec2d &x, const vec2d &v, GC_RigidBodyStatic* SpawnBy, bool advanced);
	GC_PlazmaClod(FromFile) : GC_Projectile(FromFile()) {}

	virtual bool Hit(GC_Object *object, const vec2d &hit, const vec2d &norm);
	virtual void SpawnTrailParticle(const vec2d &pos);
};

/////////////////////////////////////////////////////////////

class GC_BfgCore : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_BfgCore);
private:
	SafePtr<GC_Vehicle> _target;
	float _time;
	void FindTarget();

public:
	GC_BfgCore(const vec2d &x, const vec2d &v, GC_RigidBodyStatic* SpawnBy, bool advanced);
	GC_BfgCore(FromFile)  : GC_Projectile(FromFile()) {}
	virtual void Kill();

	virtual void Serialize(SaveFile &f);

	virtual bool Hit(GC_Object *object, const vec2d &hit, const vec2d &norm);
	virtual void SpawnTrailParticle(const vec2d &pos);

	virtual void TimeStepFixed(float dt);
};

/////////////////////////////////////////////////////////////

class GC_ACBullet : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_ACBullet);
public:
	GC_ACBullet(const vec2d &x, const vec2d &v, GC_RigidBodyStatic* SpawnBy, bool advanced);
	GC_ACBullet(FromFile);

	virtual bool Hit(GC_Object *object, const vec2d &hit, const vec2d &norm);
	virtual void SpawnTrailParticle(const vec2d &pos);
};
/////////////////////////////////////////////////////////////

class GC_GaussRay : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_GaussRay);

public:
	GC_GaussRay(const vec2d &x, const vec2d &v, GC_RigidBodyStatic* SpawnBy, bool advanced);
	GC_GaussRay(FromFile);

	virtual bool Hit(GC_Object *object, const vec2d &hit, const vec2d &norm);
	virtual void SpawnTrailParticle(const vec2d &pos);
	virtual void Kill();
};
/////////////////////////////////////////////////////////////

class GC_Weap_Ripper;

//-------------------------

class GC_Disk : public GC_Projectile
{
	DECLARE_SELF_REGISTRATION(GC_Disk);
protected:
	bool _bAttached;
	SafePtr<GC_Weap_Ripper> _ripper;

	float _time;

public:
	GC_Disk(GC_Weap_Ripper *pRipper);
	GC_Disk(const vec2d &x, const vec2d &v, GC_RigidBodyStatic* SpawnBy, bool advanced);
	GC_Disk(FromFile);
	virtual ~GC_Disk();
	virtual void Kill();

	virtual void Serialize(SaveFile &f);

	virtual bool Hit(GC_Object *object, const vec2d &hit, const vec2d &norm);
	virtual void SpawnTrailParticle(const vec2d &pos);

	virtual void TimeStepFixed(float dt);

	void OnRipperMove(GC_Object *sender, void *param);
	void OnRipperKill(GC_Object *sender, void *param);
};



///////////////////////////////////////////////////////////////////////////////
// end of file
