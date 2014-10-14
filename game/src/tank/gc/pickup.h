// pickup.h

#pragma once

#include "Actor.h"
#include "Rotator.h"
#include "constants.h"

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

#define GC_FLAG_PICKUP_BLINK             (GC_FLAG_ACTOR_ << 0)
#define GC_FLAG_PICKUP_AUTO              (GC_FLAG_ACTOR_ << 1)
#define GC_FLAG_PICKUP_RESPAWN           (GC_FLAG_ACTOR_ << 2)
#define GC_FLAG_PICKUP_KNOWNPOS          (GC_FLAG_ACTOR_ << 3)
#define GC_FLAG_PICKUP_VISIBLE           (GC_FLAG_ACTOR_ << 4)
#define GC_FLAG_PICKUP_                  (GC_FLAG_ACTOR_ << 5)

class GC_Pickup : public GC_Actor
{
    typedef GC_Actor base;

public:
	DECLARE_LIST_MEMBER();
	DECLARE_GRID_MEMBER();
	GC_Pickup(World &world);
	GC_Pickup(FromFile);
	virtual ~GC_Pickup();
	
	const std::string& GetOnPickup() const { return _scriptOnPickup; }

	void  SetRadius(float r) { _radius = r; }
	float GetRadius() const { return _radius; }
	GC_Actor* GetCarrier() const { return _pickupCarrier; }
	
	void  SetRespawnTime(float respawnTime);
	float GetRespawnTime() const;
	
	void SetRespawn(bool respawn) { SetFlags(GC_FLAG_PICKUP_RESPAWN, respawn); }
	bool GetRespawn() const       { return CheckFlags(GC_FLAG_PICKUP_RESPAWN); }
	
	void SetAutoSwitch(bool autoSwitch) { SetFlags(GC_FLAG_PICKUP_AUTO, autoSwitch); }
	bool GetAutoSwitch() const          { return CheckFlags(GC_FLAG_PICKUP_AUTO);    }
	
	void SetVisible(bool bShow) { SetFlags(GC_FLAG_PICKUP_VISIBLE, bShow); }
	bool GetVisible() const { return CheckFlags(GC_FLAG_PICKUP_VISIBLE); }
	
	float GetTimeAttached() const { assert(GetCarrier()); return _timeAttached; }
	
	void SetBlinking(bool blink);
	bool GetBlinking() const { return CheckFlags(GC_FLAG_PICKUP_BLINK); }

	// if 0 then item considered useless and will not be taken
	virtual AIPRIORITY GetPriority(World &world, const GC_Vehicle &veh) const { return AIP_NORMAL; }

	// default implementation searches for the nearest vehicle
	virtual GC_Actor* FindNewOwner(World &world) const;
	
	virtual void Attach(World &world, GC_Actor *actor);
	virtual void Detach(World &world);
	virtual void Disappear(World &world);
	virtual float GetDefaultRespawnTime() const = 0;

	// GC_Actor
    virtual void MoveTo(World &world, const vec2d &pos) override;

	// GC_Object
	virtual void Kill(World &world);
	virtual void MapExchange(World &world, MapFile &f);
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStep(World &world, float dt);
	
protected:
	class MyPropertySet : public GC_Actor::MyPropertySet
	{
		typedef GC_Actor::MyPropertySet BASE;
		ObjectProperty _propTimeRespawn;
		ObjectProperty _propOnPickup;
		
	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void MyExchange(World &world, bool applyToObject);
	};
	
	virtual PropertySet* NewPropertySet();

private:
	ObjPtr<GC_HideLabel>  _label;
	ObjPtr<GC_Actor>      _pickupCarrier;
	
	std::string  _scriptOnPickup;   // on_pickup(who)
	float  _radius;
	float  _timeAttached;
	float  _timeRespawn;

	void OnOwnerMove(World &world, GC_Object *sender, void *param);
	void OnOwnerKill(World &world, GC_Object *sender, void *param);

#ifdef NETWORK_DEBUG
	virtual DWORD checksum(void) const
	{
		DWORD cs = reinterpret_cast<const DWORD&>(GetPos().x)
		         ^ reinterpret_cast<const DWORD&>(GetPos().y)
		         ^ reinterpret_cast<const DWORD&>(_timeAttached);
		return GC_Actor::checksum() ^ cs;
	}
#endif
};

///////////////////////////////////////////////////////////////////////////////

class GC_pu_Health : public GC_Pickup
{
	DECLARE_SELF_REGISTRATION(GC_pu_Health);

public:
	GC_pu_Health(World &world);
	GC_pu_Health(FromFile);

	virtual float GetDefaultRespawnTime() const { return 15.0f; }
	virtual AIPRIORITY GetPriority(World &world, const GC_Vehicle &veh) const;

	virtual void Attach(World &world, GC_Actor *actor);
	virtual GC_Actor* FindNewOwner(World &world) const;
};

///////////////////////////////////////////////////////////////////////////////

class GC_pu_Mine : public GC_Pickup
{
	DECLARE_SELF_REGISTRATION(GC_pu_Mine);

public:
	GC_pu_Mine(World &world);
	GC_pu_Mine(FromFile);

	virtual float GetDefaultRespawnTime() const { return 15.0f; }
	virtual AIPRIORITY GetPriority(World &world, const GC_Vehicle &veh) const;

	virtual void Attach(World &world, GC_Actor *actor);
};

///////////////////////////////////////////////////////////////////////////////

class GC_pu_Shield : public GC_Pickup
{
	DECLARE_SELF_REGISTRATION(GC_pu_Shield);

private:
	float _timeHit;

public:
	GC_pu_Shield(World &world);
	GC_pu_Shield(FromFile);


	virtual float GetDefaultRespawnTime() const { return 30.0f; }
	virtual AIPRIORITY GetPriority(World &world, const GC_Vehicle &veh) const;

	virtual void Attach(World &world, GC_Actor *actor);
	virtual void Detach(World &world);

	virtual void TimeStep(World &world, float dt);
	virtual void Serialize(World &world, SaveFile &f);

protected:
	void OnOwnerDamage(World &world, GC_Object *sender, void *param);
};

///////////////////////////////////////////////////////////////////////////////

class GC_pu_Shock : public GC_Pickup
{
	DECLARE_SELF_REGISTRATION(GC_pu_Shock);
    typedef GC_Pickup base;

private:
	ObjPtr<GC_Light> _light;
	vec2d _targetPos;

	GC_Vehicle *FindNearVehicle(World &world, const GC_RigidBodyStatic *ignore) const;

public:
    DECLARE_LIST_MEMBER();
	GC_pu_Shock(World &world);
	GC_pu_Shock(FromFile);
	virtual ~GC_pu_Shock();
	
	vec2d GetTargetPos() const { return _targetPos; }

    virtual void Kill(World &world);
	virtual void Serialize(World &world, SaveFile &f);

	virtual float GetDefaultRespawnTime() const { return 15.0f; }
	virtual AIPRIORITY GetPriority(World &world, const GC_Vehicle &veh) const;

	virtual void Attach(World &world, GC_Actor *actor);
	virtual void Detach(World &world);

	virtual void TimeStep(World &world, float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_pu_Booster : public GC_Pickup
{
	DECLARE_SELF_REGISTRATION(GC_pu_Booster);

	ObjPtr<GC_Sound> _sound;

public:
	GC_pu_Booster(World &world);
	GC_pu_Booster(FromFile);
	virtual ~GC_pu_Booster();

	virtual float GetDefaultRespawnTime() const { return 30.0f; }
	virtual void Serialize(World &world, SaveFile &f);

	virtual AIPRIORITY GetPriority(World &world, const GC_Vehicle &veh) const;

	virtual void Attach(World &world, GC_Actor *actor);
	virtual void Detach(World &world);

	virtual GC_Actor* FindNewOwner(World &world) const;

	virtual void TimeStep(World &world, float dt);

	void OnWeaponDisappear(World &world, GC_Object *sender, void *param);
};


///////////////////////////////////////////////////////////////////////////////
// end of file
