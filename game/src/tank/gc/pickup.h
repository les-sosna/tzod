// pickup.h

#pragma once

#include "2dSprite.h"
#include "core/rotator.h"

/////////////////////////////////////////////////////////////
// forward declarations

class GC_HideLabel;
class GC_Vehicle;
class GC_Line;
class GC_Light;
class GC_Crosshair;

/////////////////////////////////////////////////////////////

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

/////////////////////////////////////////////////////////////

class GC_PickUp : public GC_Item
{
	MemberOfGlobalList _memberOf;

protected:
	class MyPropertySet : public IPropertySet
	{
		typedef IPropertySet BASE;

		ObjectProperty _prop_time_respawn;

	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void Exchange(bool bApply);
	};

public:
	bool	_bMostBeAllowed;	// небходимо разрешение, чтобы подобрать предмет
	float	_time;
	float	_time_animation;
	float	_time_respawn;
	bool	_bRespawn;			// респаун после того, как предмет подберут
	bool	_bAttached;		// предмет прикреплен к танку
	bool	_blink;         // item is blinking

	SafePtr<GC_Object> _ancObject;	// не респаунимс€, пока этот объект жив

public:
	GC_PickUp(float x, float y);
	GC_PickUp(FromFile);
	virtual void Kill();

	virtual void Serialize(SaveFile &f);

	void SetBlinking(bool blink);
	bool GetBlinking() const { return _blink; }


	// оценка полезности предмета дл€ данного танка.
	// если 0, то предмет бесполезен и его не нужно брать
	virtual AIPRIORITY CheckUseful(GC_Vehicle *pVehicle) {return AIP_NORMAL;};

	virtual GC_Vehicle* CheckPickUp();
	virtual void GiveIt(GC_Vehicle* pVehicle);
	virtual void Respawn();

	virtual float GetDefaultRespawnTime() const = 0;
	virtual GC_PickUp* SetRespawn() = 0;	//создать скрытую копию себ€
	void SetAnchor(GC_Object *object);

	virtual void TimeStepFixed(float dt);
	virtual void TimeStepFloat(float dt);

	virtual void Draw();

	virtual void mapExchange(MapFile &f);
	virtual IPropertySet* GetProperties();

#ifdef NETWORK_DEBUG
public:
	virtual DWORD checksum(void) const
	{
		DWORD cs = reinterpret_cast<const DWORD&>(_time);
		return GC_Item::checksum() ^ cs;
	}
#endif
};

/////////////////////////////////////////////////////////////

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

/////////////////////////////////////////////////////////////

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

/////////////////////////////////////////////////////////////

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

/////////////////////////////////////////////////////////////

class GC_pu_Shock : public GC_PickUp
{
	DECLARE_SELF_REGISTRATION(GC_pu_Shock);

private:
	SafePtr<GC_Vehicle> _vehicle;
	SafePtr<GC_Line>  _effect;
	SafePtr<GC_Light> _light;

	float _time_wait;		// врем€ ожидани€

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

/////////////////////////////////////////////////////////////

class GC_Weapon;
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

	virtual void GiveIt(GC_Vehicle* pVehicle);		//return true  -  respawn
	virtual GC_PickUp* SetRespawn();
	virtual GC_Vehicle* CheckPickUp();

	virtual void TimeStepFixed(float dt);
	virtual void TimeStepFloat(float dt);
};

/////////////////////////////////////////////////////////////

struct AIWEAPSETTINGS; // defined in ai.h

class GC_Weapon : public GC_PickUp
{
protected:
	bool _advanced;	// кваженое оружие
	SafePtr<GC_UserSprite> _fireEffect;
	SafePtr<GC_Light> _fireLight;
	vec2d _fePos;
	float _feTime;
	float _feOrient;

	virtual void UpdateView();

public:
	virtual void SetAdvanced(bool advanced) { _advanced = advanced; };
	inline  bool GetAdvanced() { return _advanced; };

public:
	float    _time;
	float    _time_reload;

	float    _angle;          // угол поворота относительно платформы
	Rotator  _rotator;

	SafePtr<GC_Vehicle>   _proprietor;
	SafePtr<GC_Crosshair> _crosshair;

	SafePtr<GC_Sound> _rotateSound;

	bool IsAdvanced() const { return _advanced; }

public:
	GC_Weapon(float x, float y);
	GC_Weapon(FromFile);
	virtual ~GC_Weapon();
	virtual void Kill();

	virtual void Serialize(SaveFile &f);

	virtual float GetDefaultRespawnTime() const { return 6.0f; }
	virtual AIPRIORITY CheckUseful(GC_Vehicle *pVehicle);
	virtual void SetupAI(AIWEAPSETTINGS *pSettings) = 0;

	virtual void GiveIt(GC_Vehicle *pVehicle);
	virtual void Attach(GC_Vehicle *pVehicle);
	virtual void Detach();
	virtual void ProcessRotate(float dt);

	virtual void SetCrosshair();

	virtual void Fire() = 0;

	virtual void TimeStepFixed(float dt);
	virtual void TimeStepFloat(float dt);

private:
	void OnProprietorMove(GC_Vehicle *sender, void *param);
};

/////////////////////////////////////////////////////////////

class GC_Weap_RocketLauncher : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_RocketLauncher);

public:
	float _time_shot;
	int _nshots_total;
	int _nshots;
	bool _firing;
	bool _reloaded;

	virtual void Attach(GC_Vehicle *pVehicle);
	virtual void Detach();

	GC_Weap_RocketLauncher(float x, float y);
	GC_Weap_RocketLauncher(FromFile);

	virtual GC_PickUp* SetRespawn();
	virtual void Serialize(SaveFile &f);
	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(float dt);
};

/////////////////////////////////////////////////////////////

class GC_Weap_AutoCannon : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_AutoCannon);

public:
	virtual void SetAdvanced(bool advanced);

public:
	float _time_shot;
	int _nshots_total;
	int _nshots;
	bool _firing;

	virtual void Attach(GC_Vehicle *pVehicle);
	virtual void Detach();

	GC_Weap_AutoCannon(float x, float y);
	GC_Weap_AutoCannon(FromFile);
	virtual ~GC_Weap_AutoCannon();
	virtual void Kill();

	virtual GC_PickUp* SetRespawn();

	virtual void Serialize(SaveFile &f);
	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(float dt);
};

/////////////////////////////////////////////////////////////

class GC_Weap_Cannon : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Cannon);

private:
	float _time_smoke;
	float _time_smoke_dt;

public:
	virtual void Attach(GC_Vehicle *pVehicle);

	GC_Weap_Cannon(float x, float y);
	GC_Weap_Cannon(FromFile);
	virtual ~GC_Weap_Cannon();

	virtual GC_PickUp* SetRespawn();

	virtual void Serialize(SaveFile &f);
	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(float dt);
};

/////////////////////////////////////////////////////////////

class GC_Weap_Plazma : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Plazma);

public:
	virtual void Attach(GC_Vehicle *pVehicle);

	GC_Weap_Plazma(float x, float y);
	GC_Weap_Plazma(FromFile) : GC_Weapon(FromFile()) {}

	virtual GC_PickUp* SetRespawn();
	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
};

/////////////////////////////////////////////////////////////

class GC_Weap_Gauss : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Gauss);

public:
	virtual void Attach(GC_Vehicle *pVehicle);

	GC_Weap_Gauss(float x, float y);
	GC_Weap_Gauss(FromFile);
	virtual ~GC_Weap_Gauss();

	virtual GC_PickUp* SetRespawn();

	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(float dt);
};

/////////////////////////////////////////////////////////////

class GC_Weap_Ram : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Ram);

private:
	SafePtr<GC_Sound> _engineSound;
	SafePtr<GC_Light> _engineLight;

protected:
	virtual void UpdateView();

public:
	float _fuel;
	float _fuel_max;
	float _fuel_rate;		// расход топлива в сек.
	float _fuel_rep;		// восстановление топлива в сек.
	bool _bFire;
	bool _bReady;

public:
	virtual void SetAdvanced(bool advanced);

public:
	virtual void Attach(GC_Vehicle *pVehicle);
	virtual void Detach();

	GC_Weap_Ram(float x, float y);
	GC_Weap_Ram(FromFile);
	virtual ~GC_Weap_Ram();
	virtual void Kill();

	virtual GC_PickUp* SetRespawn();

	virtual void Serialize(SaveFile &f);
	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(float dt);
};

/////////////////////////////////////////////////////////////

class GC_Weap_BFG : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_BFG);

private:
	float _time_ready;

public:
	virtual void Attach(GC_Vehicle *pVehicle);

	GC_Weap_BFG(float x, float y);
	GC_Weap_BFG(FromFile);
	virtual ~GC_Weap_BFG();

	virtual GC_PickUp* SetRespawn();

	virtual void Serialize(SaveFile &f);
	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(float dt);
};

/////////////////////////////////////////////////////////////

class GC_Weap_Ripper : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Ripper);

public:
	virtual void Attach(GC_Vehicle *pVehicle);

	GC_Weap_Ripper(float x, float y);
	GC_Weap_Ripper(FromFile);
	virtual ~GC_Weap_Ripper();

	virtual GC_PickUp* SetRespawn();
	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(float dt);
};

/////////////////////////////////////////////////////////////

class GC_Weap_Minigun : public GC_Weapon
{
	DECLARE_SELF_REGISTRATION(GC_Weap_Minigun);

private:
	SafePtr<GC_Sound> _sound;
	float _time_rotate;	// дл€ эмул€ции вращени€ стволов
	float _time_fire;		// врем€ непрерывнго огн€
	float _time_shot;
	bool _bFire;

	SafePtr<GC_Crosshair> _crosshair_left;

public:
	virtual void Attach(GC_Vehicle *pVehicle);
	virtual void Detach();

	GC_Weap_Minigun(float x, float y);
	GC_Weap_Minigun(FromFile);
	virtual ~GC_Weap_Minigun();
	virtual void Kill();

	virtual void SetCrosshair();

	virtual GC_PickUp* SetRespawn();

	virtual void Serialize(SaveFile &f);
	virtual void Fire();
	virtual void SetupAI(AIWEAPSETTINGS *pSettings);
	virtual void TimeStepFixed(float dt);
};

///////////////////////////////////////////////////////////////////////////////
// end of file
