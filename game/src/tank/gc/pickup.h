// pickup.h

#pragma once

#include "2dSprite.h"
#include "core/rotator.h"

///////////////////////////////////////////////////////////////////////////////
// forward declarations

class GC_HideLabel;
class GC_Vehicle;
class GC_Line;
class GC_Light;
class GC_Crosshair;
class GC_Weapon;

///////////////////////////////////////////////////////////////////////////////

class GC_Pickup : public GC_2dSprite
{
	MemberOfGlobalList _memberOf;

	class MyPropertySet : public GC_2dSprite::MyPropertySet
	{
		typedef GC_2dSprite::MyPropertySet BASE;
		ObjectProperty _propTimeRespawn;

	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void Exchange(bool applyToObject);
	};

	virtual SafePtr<PropertySet> GetProperties();

	SafePtr<GC_HideLabel> _label;
	SafePtr<GC_Actor>     _owner;

	float  _radius;
	float  _time;
	float  _timeAnimation;
	float  _timeRespawn;
	bool   _autoSwitch;
	bool   _respawn;    // flag indicates that item will be respawned in the original position
	bool   _blink;      // item is blinking

protected:
	virtual void Respawn();

	virtual void TimeStepFixed(float dt);
	virtual void TimeStepFloat(float dt);
	virtual void Kill();

	virtual void Draw();

	virtual void mapExchange(MapFile &f);
	virtual void Serialize(SaveFile &f);
	virtual bool IsSaved() { return true; }

public:
	void  SetRadius(float r)   { _radius = r;              }
	float GetRadius()    const { return _radius;           }
	GC_Actor* GetOwner() const { return GetRawPtr(_owner); }
	bool IsAttached()    const { return NULL != _owner;    }

	void  SetRespawnTime(float respawnTime);
	float GetRespawnTime() const;

	void  SetAutoSwitch(bool autoSwitch);

	// hide or kill depending on _respawn; return true if object is killed
	virtual bool Disappear();

public:
	GC_Pickup(float x, float y);
	GC_Pickup(FromFile);

	void SetBlinking(bool blink);
	bool GetBlinking() const { return _blink; }

	// оценка полезности предмета для данного танка.
	// если 0, то предмет бесполезен и его не нужно брать
	virtual AIPRIORITY CheckUseful(GC_Vehicle *veh) {return AIP_NORMAL;};
	virtual GC_Vehicle* CheckPickup();

	virtual void Attach(GC_Vehicle *vehicle);
	virtual void Detach();

	virtual float GetDefaultRespawnTime() const = 0;

#ifdef NETWORK_DEBUG
public:
	virtual DWORD checksum(void) const
	{
		DWORD cs = reinterpret_cast<const DWORD&>(GetPos().x) 
		         ^ reinterpret_cast<const DWORD&>(GetPos().y)
		         ^ reinterpret_cast<const DWORD&>(_time);
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
	virtual AIPRIORITY CheckUseful(GC_Vehicle *veh);

	virtual void Attach(GC_Vehicle* veh);
	virtual GC_Vehicle* CheckPickup();
};

///////////////////////////////////////////////////////////////////////////////

class GC_pu_Mine : public GC_Pickup
{
	DECLARE_SELF_REGISTRATION(GC_pu_Mine);

public:
	GC_pu_Mine(float x, float y);
	GC_pu_Mine(FromFile);

	virtual float GetDefaultRespawnTime() const { return 15.0f; }
	virtual AIPRIORITY CheckUseful(GC_Vehicle *veh);

	virtual void Attach(GC_Vehicle* veh);
	virtual GC_Vehicle* CheckPickup();
};

///////////////////////////////////////////////////////////////////////////////

class GC_pu_Invulnerablity : public GC_Pickup
{
	DECLARE_SELF_REGISTRATION(GC_pu_Invulnerablity);

private:
	float _timeHit;

public:
	GC_pu_Invulnerablity(float x, float y);
	GC_pu_Invulnerablity(FromFile);

	virtual void Serialize(SaveFile &f);

	virtual float GetDefaultRespawnTime() const { return 30.0f; }
	virtual AIPRIORITY CheckUseful(GC_Vehicle *veh);

	virtual void Attach(GC_Vehicle* veh);

	virtual void TimeStepFixed(float dt);
	virtual void TimeStepFloat(float dt);

public:
	void OnOwnerDamage(GC_Object *sender, void *param);
	void OnOwnerMove(GC_Object *sender, void *param);
};

///////////////////////////////////////////////////////////////////////////////

class GC_pu_Shock : public GC_Pickup
{
	DECLARE_SELF_REGISTRATION(GC_pu_Shock);

private:
	SafePtr<GC_Line>  _effect;
	SafePtr<GC_Light> _light;

	float _timeout;

	GC_Vehicle *FindNearVehicle(GC_Vehicle *pIgnore);

public:
	GC_pu_Shock(float x, float y);
	GC_pu_Shock(FromFile);
	virtual ~GC_pu_Shock();
	virtual void Kill();

	virtual void Serialize(SaveFile &f);

	virtual float GetDefaultRespawnTime() const { return 15.0f; }
	virtual AIPRIORITY CheckUseful(GC_Vehicle *veh);

	virtual void Attach(GC_Vehicle* veh);

	virtual void TimeStepFixed(float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_pu_Booster : public GC_Pickup
{
	DECLARE_SELF_REGISTRATION(GC_pu_Booster);

public:
	GC_pu_Booster(float x, float y);
	GC_pu_Booster(FromFile);
	virtual ~GC_pu_Booster();
	virtual bool Disappear();

	virtual float GetDefaultRespawnTime() const { return 30.0f; }
	virtual void Serialize(SaveFile &f);

	virtual AIPRIORITY CheckUseful(GC_Vehicle *veh);

	virtual void Attach(GC_Vehicle* veh);
	virtual void Detach();

	virtual GC_Vehicle* CheckPickup();

	virtual void TimeStepFixed(float dt);
	virtual void TimeStepFloat(float dt);

	void OnWeaponDisappear(GC_Object *sender, void *param);
};


///////////////////////////////////////////////////////////////////////////////
// end of file
