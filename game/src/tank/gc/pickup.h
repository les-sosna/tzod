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

class GC_Item : public GC_2dSprite
{
	float                 _radius;  // радиус предмета
	SafePtr<GC_HideLabel> _label;   // указатель на метку

public:
	void  setRadius(float r) { _radius = r;    }
	float getRadius() const  { return _radius; }

public:
	GC_Item(float x, float y);
	GC_Item(FromFile);

	virtual bool IsSaved() { return true; }
	virtual void Serialize(SaveFile &f);
	virtual void Kill();

#ifdef NETWORK_DEBUG
public:
	virtual DWORD checksum(void) const
	{
		DWORD cs = reinterpret_cast<const DWORD&>(_pos.x) ^ reinterpret_cast<const DWORD&>(_pos.y);
		return GC_2dSprite::checksum() ^ cs;
	}
#endif
};

///////////////////////////////////////////////////////////////////////////////

class GC_PickUp : public GC_Item
{
	MemberOfGlobalList _memberOf;

protected:
	class MyPropertySet : public PropertySet
	{
		typedef PropertySet BASE;
		ObjectProperty _propTimeRespawn;

	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void Exchange(bool bApply);
	};

public:
	bool	_bMostBeAllowed; // небходимо разрешение, чтобы подобрать предмет
	float	_time;
	float	_time_animation;
	float	_time_respawn;
	bool	_bRespawn;      // респаун после того, как предмет подберут
	bool	_attached;     // предмет прикреплен к танку
	bool	_blink;         // item is blinking

	SafePtr<GC_Object> _ancObject; // не респаунимся, пока этот объект жив

public:
	GC_PickUp(float x, float y);
	GC_PickUp(FromFile);
	virtual void Kill();

	virtual void Serialize(SaveFile &f);

	void SetBlinking(bool blink);
	bool GetBlinking() const { return _blink; }

	bool IsAttached() const { return _attached; }



	// оценка полезности предмета для данного танка.
	// если 0, то предмет бесполезен и его не нужно брать
	virtual AIPRIORITY CheckUseful(GC_Vehicle *pVehicle) {return AIP_NORMAL;};

	virtual GC_Vehicle* CheckPickUp();
	virtual void GiveIt(GC_Vehicle* pVehicle);
	virtual void Respawn();

	virtual float GetDefaultRespawnTime() const = 0;
	virtual GC_PickUp* SetRespawn() = 0; //создать скрытую копию себя
	void SetAnchor(GC_Object *object);

	virtual void TimeStepFixed(float dt);
	virtual void TimeStepFloat(float dt);

	virtual void Draw();

	virtual void mapExchange(MapFile &f);
	virtual SafePtr<PropertySet> GetProperties();

#ifdef NETWORK_DEBUG
public:
	virtual DWORD checksum(void) const
	{
		DWORD cs = reinterpret_cast<const DWORD&>(_time);
		return GC_Item::checksum() ^ cs;
	}
#endif
};

///////////////////////////////////////////////////////////////////////////////

class GC_pu_Health : public GC_PickUp
{
	DECLARE_SELF_REGISTRATION(GC_pu_Health);

public:
	GC_pu_Health(float x, float y);
	GC_pu_Health(FromFile);

	virtual float GetDefaultRespawnTime() const { return 15.0f; }
	virtual AIPRIORITY CheckUseful(GC_Vehicle *pVehicle);

	virtual void GiveIt(GC_Vehicle* pVehicle);
	virtual GC_PickUp* SetRespawn();
	virtual GC_Vehicle* CheckPickUp();
};

///////////////////////////////////////////////////////////////////////////////

class GC_pu_Mine : public GC_PickUp
{
	DECLARE_SELF_REGISTRATION(GC_pu_Mine);

public:
	GC_pu_Mine(float x, float y);
	GC_pu_Mine(FromFile);

	virtual float GetDefaultRespawnTime() const { return 15.0f; }
	virtual AIPRIORITY CheckUseful(GC_Vehicle *pVehicle);

	virtual void GiveIt(GC_Vehicle* pVehicle);
	virtual GC_PickUp* SetRespawn();
	virtual GC_Vehicle* CheckPickUp();
};

///////////////////////////////////////////////////////////////////////////////

class GC_pu_Invulnerablity : public GC_PickUp
{
	DECLARE_SELF_REGISTRATION(GC_pu_Invulnerablity);

private:
	float _time_hit;

public:
	GC_pu_Invulnerablity(float x, float y);
	GC_pu_Invulnerablity(FromFile);

	virtual void Serialize(SaveFile &f);

	virtual float GetDefaultRespawnTime() const { return 30.0f; }
	virtual AIPRIORITY CheckUseful(GC_Vehicle *pVehicle);

	virtual void GiveIt(GC_Vehicle* pVehicle);
	virtual GC_PickUp* SetRespawn();

	virtual void TimeStepFixed(float dt);
	virtual void TimeStepFloat(float dt);

public:
	void OnProprietorDamage(GC_Object *sender, void *param);
	void OnProprietorMove(GC_Object *sender, void *param);
};

///////////////////////////////////////////////////////////////////////////////

class GC_pu_Shock : public GC_PickUp
{
	DECLARE_SELF_REGISTRATION(GC_pu_Shock);

private:
	SafePtr<GC_Vehicle> _vehicle;
	SafePtr<GC_Line>  _effect;
	SafePtr<GC_Light> _light;

	float _time_wait; // время ожидания

	GC_Vehicle *FindNearVehicle(GC_Vehicle *pIgnore);

public:
	GC_pu_Shock(float x, float y);
	GC_pu_Shock(FromFile);
	virtual ~GC_pu_Shock();
	virtual void Kill();

	virtual void Serialize(SaveFile &f);

	virtual float GetDefaultRespawnTime() const { return 15.0f; }
	virtual AIPRIORITY CheckUseful(GC_Vehicle *pVehicle);

	virtual void GiveIt(GC_Vehicle* pVehicle);
	virtual GC_PickUp* SetRespawn();

	virtual void TimeStepFixed(float dt);
};

///////////////////////////////////////////////////////////////////////////////

class GC_pu_Booster : public GC_PickUp
{
	DECLARE_SELF_REGISTRATION(GC_pu_Booster);

protected:
	SafePtr<GC_Weapon> _weapon;

public:
	GC_pu_Booster(float x, float y);
	GC_pu_Booster(FromFile);
	virtual ~GC_pu_Booster();
	virtual void Kill();

	virtual float GetDefaultRespawnTime() const { return 30.0f; }
	virtual void Serialize(SaveFile &f);

	virtual AIPRIORITY CheckUseful(GC_Vehicle *pVehicle);

	virtual void GiveIt(GC_Vehicle* pVehicle); //return true  -  respawn
	virtual GC_PickUp* SetRespawn();
	virtual GC_Vehicle* CheckPickUp();

	virtual void TimeStepFixed(float dt);
	virtual void TimeStepFloat(float dt);
};


///////////////////////////////////////////////////////////////////////////////
// end of file
