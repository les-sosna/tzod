#pragma once
#include "RigidBodyDinamic.h"
#include "VehicleState.h"

class GC_Player;
class GC_Weapon;
class GC_DamLabel;
class GC_Sound;
class GC_Light;


struct VehicleClass
{
	std::string displayName;

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


class GC_Vehicle : public GC_RigidBodyDynamic
{
    typedef GC_RigidBodyDynamic base;
    
	ObjPtr<GC_Weapon>   _weapon;
	ObjPtr<GC_Player>   _player;

public:
    DECLARE_LIST_MEMBER();
	GC_Vehicle(World &world);
	GC_Vehicle(FromFile);
	virtual ~GC_Vehicle();

	void SetClass(const VehicleClass &vc); // apply vehicle class
	void SetMaxHP(float hp);

	float _enginePower;
	float _rotatePower;
	float _maxRotSpeed;
	float _maxLinSpeed;

protected:
	void ApplyState(const VehicleState &vs);


public:
	VehicleState _state;

	float GetMaxSpeed() const;
	float GetMaxBrakingLength() const;

	void SetMoveSound(World &world, enumSoundTemplate s);

	GC_Weapon* GetWeapon() const { return _weapon; }

	void SetPlayer(World &world, GC_Player *player);

public:
	ObjPtr<GC_Sound>    _moveSound;
	ObjPtr<GC_DamLabel> _damLabel;
    
	ObjPtr<GC_Light>    _light_ambient;
	ObjPtr<GC_Light>    _light1;
	ObjPtr<GC_Light>    _light2;
    
	float _trackDensity;
	float _trackPathL;
	float _trackPathR;
	float _time_smoke;
    
	void UpdateLight(World &world);

	void ResetClass();
	void SetSkin(const std::string &skin);
	void SetControllerState(const VehicleState &vs);

	// GC_RigidBodyStatic
	virtual bool TakeDamage(World &world, float damage, const vec2d &hit, GC_Player *from);
	virtual unsigned char GetPassability() const { return 0; } // not an obstacle
	virtual GC_Player* GetOwner() const { return _player; }

	// GC_Actor
	virtual void OnPickup(World &world, GC_Pickup *pickup, bool attached);

	// GC_2dSprite
	virtual void Draw(DrawingContext &dc, bool editorMode) const;

	// GC_Object
	virtual void Kill(World &world);
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStepFixed(World &world, float dt);
	virtual void TimeStepFloat(World &world, float dt);
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
	GC_Tank_Light(World &world);
	GC_Tank_Light(FromFile);

	virtual float GetDefaultHealth() const { return 100; }
	virtual void OnDestroy(World &world) override;
};

// end of file
