// pickup.h

#pragma once

#include "2dSprite.h"
#include "core/rotator.h"


typedef float AIPRIORITY;

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
		virtual void MyExchange(Level &world, bool applyToObject);
	};

	virtual PropertySet* NewPropertySet();

private:
	ObjPtr<GC_HideLabel>  _label;
	ObjPtr<GC_Actor>      _pickupCarrier;

	std::string  _scriptOnPickup;   // on_pickup(who)
	float  _radius;
	float  _timeAttached;
	float  _timeAnimation;
	float  _timeRespawn;

protected:
	virtual void Respawn(Level &world);

	virtual void TimeStepFixed(Level &world, float dt);
	virtual void TimeStepFloat(Level &world, float dt);
	virtual void Kill(Level &world);

	virtual void Draw(bool editorMode) const;

	virtual void MapExchange(Level &world, MapFile &f);
	virtual void Serialize(Level &world, SaveFile &f);

public:
	void  SetRadius(float r)   { _radius = r;              }
	float GetRadius()    const { return _radius;           }
	GC_Actor* GetCarrier() const { return _pickupCarrier; }

	void  SetRespawnTime(float respawnTime);
	float GetRespawnTime() const;

	void SetRespawn(bool respawn) { SetFlags(GC_FLAG_PICKUP_RESPAWN, respawn); }
	bool GetRespawn() const       { return CheckFlags(GC_FLAG_PICKUP_RESPAWN); }

	void SetAutoSwitch(bool autoSwitch) { SetFlags(GC_FLAG_PICKUP_AUTO, autoSwitch); }
	bool GetAutoSwitch() const          { return CheckFlags(GC_FLAG_PICKUP_AUTO);    }

	float GetTimeAnimation() const { return _timeAnimation; }
	float GetTimeAttached() const { assert(GetCarrier()); return _timeAttached; }

	// hide or kill depending on _respawn; return true if object is killed
	virtual bool Disappear(Level &world);

public:
	GC_Pickup(Level &world, float x, float y);
	GC_Pickup(FromFile);
	virtual ~GC_Pickup();

	void SetBlinking(bool blink);
	bool GetBlinking() const { return CheckFlags(GC_FLAG_PICKUP_BLINK); }

	// if 0 then item considered useless and will not be taken
	virtual AIPRIORITY GetPriority(const GC_Vehicle &veh) const { return AIP_NORMAL; }

	// default implementation searches for the nearest vehicle
	virtual GC_Actor* FindNewOwner(Level &world) const;

	virtual void Attach(Level &world, GC_Actor *actor);
	virtual void Detach(Level &world);

	virtual float GetDefaultRespawnTime() const = 0;

protected:
	void OnOwnerMove(Level &world, GC_Object *sender, void *param);
	void OnOwnerKill(Level &world, GC_Object *sender, void *param);

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
	GC_pu_Health(Level &world, float x, float y);
	GC_pu_Health(FromFile);

	virtual float GetDefaultRespawnTime() const { return 15.0f; }
	virtual AIPRIORITY GetPriority(const GC_Vehicle &veh) const;

	virtual void Attach(Level &world, GC_Actor *actor);
	virtual GC_Actor* FindNewOwner(Level &world) const;
};

///////////////////////////////////////////////////////////////////////////////

class GC_pu_Mine : public GC_Pickup
{
	DECLARE_SELF_REGISTRATION(GC_pu_Mine);

public:
	GC_pu_Mine(Level &world, float x, float y);
	GC_pu_Mine(FromFile);

	virtual float GetDefaultRespawnTime() const { return 15.0f; }
	virtual AIPRIORITY GetPriority(const GC_Vehicle &veh) const;

	virtual void Attach(Level &world, GC_Actor *actor);
};

///////////////////////////////////////////////////////////////////////////////

class GC_pu_Shield : public GC_Pickup
{
	DECLARE_SELF_REGISTRATION(GC_pu_Shield);

private:
	float _timeHit;

public:
	GC_pu_Shield(Level &world, float x, float y);
	GC_pu_Shield(FromFile);

	virtual void Serialize(Level &world, SaveFile &f);

	virtual float GetDefaultRespawnTime() const { return 30.0f; }
	virtual AIPRIORITY GetPriority(const GC_Vehicle &veh) const;

	virtual void Attach(Level &world, GC_Actor *actor);
	virtual void Detach(Level &world);

	virtual void TimeStepFixed(Level &world, float dt);
	virtual void TimeStepFloat(Level &world, float dt);

protected:
	void OnOwnerDamage(Level &world, GC_Object *sender, void *param);
};

///////////////////////////////////////////////////////////////////////////////

class GC_pu_Shock : public GC_Pickup
{
	DECLARE_SELF_REGISTRATION(GC_pu_Shock);

private:
	ObjPtr<GC_Light> _light;
	vec2d _targetPosPredicted;

	GC_Vehicle *FindNearVehicle(Level &world, const GC_RigidBodyStatic *ignore) const;

public:
	GC_pu_Shock(Level &world, float x, float y);
	GC_pu_Shock(FromFile);
	virtual ~GC_pu_Shock();

    virtual void Kill(Level &world);
	virtual void Serialize(Level &world, SaveFile &f);

	virtual float GetDefaultRespawnTime() const { return 15.0f; }
	virtual AIPRIORITY GetPriority(const GC_Vehicle &veh) const;

	virtual void Attach(Level &world, GC_Actor *actor);
	virtual void Detach(Level &world);

	virtual void TimeStepFixed(Level &world, float dt);
	virtual void Draw(bool editorMode) const;
};

///////////////////////////////////////////////////////////////////////////////

class GC_pu_Booster : public GC_Pickup
{
	DECLARE_SELF_REGISTRATION(GC_pu_Booster);

	ObjPtr<GC_Sound> _sound;

public:
	GC_pu_Booster(Level &world, float x, float y);
	GC_pu_Booster(FromFile);
	virtual ~GC_pu_Booster();

	virtual float GetDefaultRespawnTime() const { return 30.0f; }
	virtual void Serialize(Level &world, SaveFile &f);

	virtual AIPRIORITY GetPriority(const GC_Vehicle &veh) const;

	virtual void Attach(Level &world, GC_Actor *actor);
	virtual void Detach(Level &world);

	virtual GC_Actor* FindNewOwner(Level &world) const;

	virtual void TimeStepFixed(Level &world, float dt);
	virtual void TimeStepFloat(Level &world, float dt);

	void OnWeaponDisappear(Level &world, GC_Object *sender, void *param);
};


///////////////////////////////////////////////////////////////////////////////
// end of file
