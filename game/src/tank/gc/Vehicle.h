// Vehicle.h

#pragma once

#include "RigidBodyDinamic.h"
#include "network/ControlPacket.h"

/////////////////////////////////////////////////////////////

class GC_Player;
class GC_Weapon;
class GC_DamLabel;
class GC_Sound;
class GC_Light;


struct VehicleClass
{
	string_t displayName;

	float health;
	float percussion;
	float fragility;

	float length;
	float width;

	float m, i;

	float _Nx;      // dry friction factor X
	float _Ny;      // dry friction factor Y
	float _Nw;      // angular dry friction factor

	float _Mx;      // viscous friction factor X
	float _My;      // viscous friction factor Y
	float _Mw;      // angular viscous friction factor

	float enginePower;
	float rotatePower;

	float maxRotSpeed;
	float maxLinSpeed;
};

#define GC_FLAG_VEHICLEBASE_          (GC_FLAG_RBDYMAMIC_ << 0)


class GC_VehicleBase : public GC_RigidBodyDynamic
{
public:
	GC_VehicleBase();
	GC_VehicleBase(FromFile);
	virtual ~GC_VehicleBase();

	void SetClass(const VehicleClass &vc); // apply vehicle class
	void SetMaxHP(float hp);

	float _enginePower;
	float _rotatePower;
	float _maxRotSpeed;
	float _maxLinSpeed;

	// GC_Object
	virtual void Serialize(SaveFile &f);

protected:
	void ApplyState(const VehicleState &vs);
};

// forward declaration
class GC_VehicleVisualDummy;

class GC_Vehicle : public GC_VehicleBase
{
	MemberOfGlobalList<LIST_vehicles> _memberOf;

	SafePtr<GC_Weapon>   _weapon;
	SafePtr<GC_Player>   _player;

protected:
	SafePtr<GC_VehicleVisualDummy> _visual;

public:
	VehicleState _stateReal;
	VehicleState _statePredicted;

	float GetMaxSpeed() const;
	float GetMaxBrakingLength() const;

	GC_Weapon* GetWeapon() const { return GetRawPtr(_weapon); }
	GC_Player* GetPlayer() const { return GetRawPtr(_player); }

	void SetPlayer(SafePtr<GC_Player> &player);

public:
	GC_Vehicle(float x, float y);
	GC_Vehicle(FromFile);
	virtual ~GC_Vehicle();

	void ResetClass();
	void SetSkin(const string_t &skin);
	void SetState(const VehicleState &vs);
	void SetPredictedState(const VehicleState &vs);
	const VehicleState& GetPredictedState() const { return _statePredicted; }
	GC_VehicleVisualDummy* GetVisual() const { return GetRawPtr(_visual); }
	void SetBodyAngle(float a);

	// GC_RigidBodyStatic
	virtual bool TakeDamage(float damage, const vec2d &hit, GC_RigidBodyStatic *from);
	virtual unsigned char GetPassability() const { return 0; } // не является препятствием

	// GC_Actor
	virtual const vec2d& GetPosPredicted() const;
	virtual void OnPickup(GC_Pickup *pickup, bool attached);

	// GC_2dSprite
#ifdef _DEBUG
	virtual void Draw() const;
#endif

	// GC_Object
	virtual void Kill();
	virtual void Serialize(SaveFile &f);
	virtual void TimeStepFixed(float dt);
#ifdef NETWORK_DEBUG
public:
	virtual DWORD checksum(void) const
	{
		DWORD cs = reinterpret_cast<const DWORD&>(_enginePower) ^ reinterpret_cast<const DWORD&>(_rotatePower);
		cs ^= reinterpret_cast<const DWORD&>(_maxRotSpeed) ^ reinterpret_cast<const DWORD&>(_maxLinSpeed);
		return GC_RigidBodyDynamic::checksum() ^ cs;
	}
#endif

protected:
	virtual bool Ignore(GC_RigidBodyStatic *test) const { return _visual == test; }
};

///////////////////////////////////////////////////////////////////////////////

#define GC_FLAG_VEHICLEDUMMY_TRACKS      (GC_FLAG_VEHICLEBASE_ << 0)
#define GC_FLAG_VEHICLEDUMMY_            (GC_FLAG_VEHICLEBASE_ << 1)


class GC_VehicleVisualDummy : public GC_VehicleBase
{
	DECLARE_SELF_REGISTRATION(GC_VehicleVisualDummy);

public:
	GC_VehicleVisualDummy(GC_Vehicle *parent);
	GC_VehicleVisualDummy(FromFile);
	virtual ~GC_VehicleVisualDummy();

	void SetMoveSound(enumSoundTemplate s);

	// GC_RigidBodyStatic
	virtual unsigned char GetPassability() const { return 0; } // не является препятствием
	virtual float GetDefaultHealth() const { return 1; }
	virtual bool TakeDamage(float damage, const vec2d &hit, GC_RigidBodyStatic *from);

	// GC_2dSprite
	virtual void Draw() const;

	// GC_Object
	virtual void Kill();
	virtual void Serialize(SaveFile &f);
	virtual void TimeStepFixed(float dt);
	virtual void TimeStepFloat(float dt);
#ifdef NETWORK_DEBUG
public:
	virtual DWORD checksum(void) const
	{
		return 0;
	}
#endif

protected:
	virtual bool Ignore(GC_RigidBodyStatic *test) const { return _parent == test; }

protected:
	SafePtr<GC_Vehicle>  _parent;
	SafePtr<GC_Sound>    _moveSound;
	SafePtr<GC_DamLabel> _damLabel;

	SafePtr<GC_Light>    _light_ambient;
	SafePtr<GC_Light>    _light1;
	SafePtr<GC_Light>    _light2;

	float _trackDensity;
	float _trackPathL;
	float _trackPathR;
	float _time_smoke;

	void UpdateLight();

	void OnDamageParent(GC_Object *sender, void *param);
	void OnDamLabelDisappear(GC_Object *sender, void *param);
};

/////////////////////////////////////////////////////////////

class GC_Tank_Light : public GC_Vehicle
{
	DECLARE_SELF_REGISTRATION(GC_Tank_Light);

public:
	GC_Tank_Light(float x, float y);
	GC_Tank_Light(FromFile);

	virtual float GetDefaultHealth() const { return 100; }
	virtual void OnDestroy();
};

// end of file
