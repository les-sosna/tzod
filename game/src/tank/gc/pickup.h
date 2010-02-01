// pickup.h

#pragma once

#include "2dSprite.h"
#include "core/rotator.h"

///////////////////////////////////////////////////////////////////////////////
// forward declarations

class GC_HideLabel;
class GC_Vehicle;
class GC_Light;
class GC_Crosshair;
class GC_Weapon;
class GC_RigidBodyStatic;

///////////////////////////////////////////////////////////////////////////////

#define GC_FLAG_PICKUP_BLINK             (GC_FLAG_2DSPRITE_ << 0)
#define GC_FLAG_PICKUP_AUTO              (GC_FLAG_2DSPRITE_ << 1)
#define GC_FLAG_PICKUP_RESPAWN           (GC_FLAG_2DSPRITE_ << 2)
#define GC_FLAG_PICKUP_                  (GC_FLAG_2DSPRITE_ << 3)

class GC_Pickup : public GC_2dSprite
{
	MemberOfGlobalList<LIST_pickups> _memberOf;

protected:
	class MyPropertySet : public GC_2dSprite::MyPropertySet
	{
		typedef GC_2dSprite::MyPropertySet BASE;
		ObjectProperty _propTimeRespawn;
		ObjectProperty _propOnPickup;

	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void MyExchange(bool applyToObject);
	};

	virtual PropertySet* NewPropertySet();

private:
	SafePtr<GC_HideLabel>  _label;
	SafePtr<GC_Actor>      _pickupCarrier;

	string_t  _scriptOnPickup;   // on_pickup(who)
	float     _radius;
	float  _timeAttached;
	float  _timeAnimation;
	float  _timeRespawn;

protected:
	virtual void Respawn();

	virtual void TimeStepFixed(float dt);
	virtual void TimeStepFloat(float dt);
	virtual void Kill();

	virtual void Draw() const;

	virtual void MapExchange(MapFile &f);
	virtual void Serialize(SaveFile &f);

public:
	void  SetRadius(float r)   { _radius = r;              }
	float GetRadius()    const { return _radius;           }
	GC_Actor* GetCarrier() const { return GetRawPtr(_pickupCarrier); }

	void  SetRespawnTime(float respawnTime);
	float GetRespawnTime() const;

	void SetRespawn(bool respawn) { SetFlags(GC_FLAG_PICKUP_RESPAWN, respawn); }
	bool GetRespawn() const       { return CheckFlags(GC_FLAG_PICKUP_RESPAWN); }

	void SetAutoSwitch(bool autoSwitch) { SetFlags(GC_FLAG_PICKUP_AUTO, autoSwitch); }
	bool GetAutoSwitch() const          { return CheckFlags(GC_FLAG_PICKUP_AUTO);    }

	float GetTimeAnimation() const { return _timeAnimation; }
	float GetTimeAttached() const { assert(GetCarrier()); return _timeAttached; }

	// hide or kill depending on _respawn; return true if object is killed
	virtual bool Disappear();

public:
	GC_Pickup(float x, float y);
	GC_Pickup(FromFile);
	virtual ~GC_Pickup();

	void SetBlinking(bool blink);
	bool GetBlinking() const { return CheckFlags(GC_FLAG_PICKUP_BLINK); }

	// if 0 then item considered useless and will not be taken
	virtual AIPRIORITY GetPriority(GC_Vehicle *veh) { return AIP_NORMAL; }

	// default implementation searches for the nearest vehicle
	virtual GC_Actor* FindNewOwner() const;

	virtual void Attach(GC_Actor *actor);
	virtual void Detach();

	virtual float GetDefaultRespawnTime() const = 0;

protected:
	void OnOwnerMove(GC_Object *sender, void *param);
	void OnOwnerKill(GC_Object *sender, void *param);

public:
	virtual const vec2d& GetPosPredicted() const { return GetCarrier() ? GetCarrier()->GetPosPredicted() : GetPos(); }

#ifdef NETWORK_DEBUG
	virtual DWORD checksum(void) const
	{
		DWORD cs = reinterpret_cast<const DWORD&>(GetPos().x)
		         ^ reinterpret_cast<const DWORD&>(GetPos().y)
		         ^ reinterpret_cast<const DWORD&>(_timeAttached);
		return GC_2dSprite::checksum() ^ cs;
	}
#endif
};

///////////////////////////////////////////////////////////////////////////////

class GC_pu_Health : public GC_Pickup
{
	DECLARE_SELF_REGISTRATION(GC_pu_Health);

public:
	GC_pu_Health(float x, float y);
	GC_pu_Health(FromFile);

	virtual float GetDefaultRespawnTime() const { return 15.0f; }
	virtual AIPRIORITY GetPriority(GC_Vehicle *veh);

	virtual void Attach(GC_Actor *actor);
	virtual GC_Actor* FindNewOwner() const;
};

///////////////////////////////////////////////////////////////////////////////

class GC_pu_Mine : public GC_Pickup
{
	DECLARE_SELF_REGISTRATION(GC_pu_Mine);

public:
	GC_pu_Mine(float x, float y);
	GC_pu_Mine(FromFile);

	virtual float GetDefaultRespawnTime() const { return 15.0f; }
	virtual AIPRIORITY GetPriority(GC_Vehicle *veh);

	virtual void Attach(GC_Actor *actor);
};

///////////////////////////////////////////////////////////////////////////////

class GC_pu_Shield : public GC_Pickup
{
	DECLARE_SELF_REGISTRATION(GC_pu_Shield);

private:
	float _timeHit;

public:
	GC_pu_Shield(float x, float y);
	GC_pu_Shield(FromFile);

	virtual void Serialize(SaveFile &f);

	virtual float GetDefaultRespawnTime() const { return 30.0f; }
	virtual AIPRIORITY GetPriority(GC_Vehicle *veh);

	virtual void Attach(GC_Actor *actor);
	virtual void Detach();

	virtual void TimeStepFixed(float dt);
	virtual void TimeStepFloat(float dt);

protected:
	void OnOwnerDamage(GC_Object *sender, void *param);
};

///////////////////////////////////////////////////////////////////////////////

class GC_pu_Shock : public GC_Pickup
{
	DECLARE_SELF_REGISTRATION(GC_pu_Shock);

private:
	SafePtr<GC_Light> _light;
	vec2d _targetPosPredicted;

	GC_Vehicle *FindNearVehicle(const GC_RigidBodyStatic *ignore);

public:
	GC_pu_Shock(float x, float y);
	GC_pu_Shock(FromFile);
	virtual ~GC_pu_Shock();
	virtual void Kill();

	virtual void Serialize(SaveFile &f);

	virtual float GetDefaultRespawnTime() const { return 15.0f; }
	virtual AIPRIORITY GetPriority(GC_Vehicle *veh);

	virtual void Attach(GC_Actor *actor);
	virtual void Detach();

	virtual void TimeStepFixed(float dt);
	virtual void Draw() const;
};

///////////////////////////////////////////////////////////////////////////////

class GC_pu_Booster : public GC_Pickup
{
	DECLARE_SELF_REGISTRATION(GC_pu_Booster);

	SafePtr<GC_Sound> _sound;

public:
	GC_pu_Booster(float x, float y);
	GC_pu_Booster(FromFile);
	virtual ~GC_pu_Booster();

	virtual float GetDefaultRespawnTime() const { return 30.0f; }
	virtual void Serialize(SaveFile &f);

	virtual AIPRIORITY GetPriority(GC_Vehicle *veh);

	virtual void Attach(GC_Actor *actor);
	virtual void Detach();

	virtual GC_Actor* FindNewOwner() const;

	virtual void TimeStepFixed(float dt);
	virtual void TimeStepFloat(float dt);

	void OnWeaponDisappear(GC_Object *sender, void *param);
};


///////////////////////////////////////////////////////////////////////////////
// end of file
