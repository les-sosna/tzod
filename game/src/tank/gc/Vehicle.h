// Vehicle.h

#pragma once

#include "RigidBodyDinamic.h"

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

	vec2d bounds[4];

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

struct VehicleState
{
	union {
		struct {
			bool _bState_MoveForward : 1;
			bool _bState_MoveBack    : 1;
			bool _bExplicitBody      : 1;
			bool _bState_Fire        : 1;
			bool _bState_AllowDrop   : 1;
			bool _bLight             : 1;
			bool _bExplicitTower     : 1;
		};
		BYTE flags;
	};
	union {
		struct {
			bool _bState_RotateLeft;
			bool _bState_RotateRight;
		};
		float _fBodyAngle;
	};
	union {
		struct {
			bool _bState_TowerLeft;
			bool _bState_TowerRight;
			bool _bState_TowerCenter;
		};
		float _fTowerAngle;
	};
};

//----------------------------------------------------------

#define MODE_EXPLICITTOWER  0x4000
#define MODE_EXPLICITBODY   0x8000

//----------------------------------------------------------

class GC_Vehicle : public GC_RigidBodyDynamic
{
	MemberOfGlobalList<LIST_vehicles> _memberOf;

	float _time_smoke;

	SafePtr<GC_Weapon>   _weapon;
	SafePtr<GC_Player>   _player;

public:
	SafePtr<GC_Sound>    _moveSound;
	SafePtr<GC_DamLabel> _damLabel;

protected:
	SafePtr<GC_Light>    _light_ambient;
	SafePtr<GC_Light>    _light1;
	SafePtr<GC_Light>    _light2;

	float _trackDensity;
	float _trackPathL;
	float _trackPathR;

	void UpdateLight();

public:
	void SetClass(const VehicleClass &vc); // apply vehicle class
	void ResetClass();

	float _enginePower;
	float _rotatePower;
	float _maxRotSpeed;
	float _maxLinSpeed;

	VehicleState _state;

	float GetMaxSpeed() const { return 100; }

	GC_Weapon* GetWeapon() const { return GetRawPtr(_weapon); }
	GC_Player* GetPlayer() const { return GetRawPtr(_player); }

	void SetPlayer(GC_Player *player);

public:
	GC_Vehicle(float x, float y);
	GC_Vehicle(FromFile);
	virtual ~GC_Vehicle();
	virtual void Kill();

	virtual void Serialize(SaveFile &f);

	virtual unsigned char GetPassability() const { return 0; } // не является препятствием

	void SetSkin(const char *skin);
	void SetState(const VehicleState &vs);

	void SetMoveSound(enumSoundTemplate s);

	virtual bool TakeDamage(float damage, const vec2d &hit, GC_RigidBodyStatic *from);

	void SetMaxHP(float hp);

	virtual void TimeStepFixed(float dt);
	virtual void Draw();

	virtual void OnPickup(GC_Pickup *pickup, bool attached);

	//--------------------------------

#ifdef NETWORK_DEBUG
public:
	virtual DWORD checksum(void) const
	{
		DWORD cs = reinterpret_cast<const DWORD&>(_enginePower) ^ reinterpret_cast<const DWORD&>(_rotatePower);
		cs ^= reinterpret_cast<const DWORD&>(_maxRotSpeed) ^ reinterpret_cast<const DWORD&>(_maxLinSpeed);
		return GC_RigidBodyDynamic::checksum() ^ cs;
	}
#endif

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
