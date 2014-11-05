#pragma once
#include "RigidBodyDinamic.h"
#include "VehicleState.h"

class GC_Player;
class GC_Weapon;
class GC_DamLabel;
class GC_Light;
class GC_pu_Shield;
struct VehicleClass;

#define GC_FLAG_VEHICLE_KNOWNLIGHT     (GC_FLAG_RBDYMAMIC_ << 0)
#define GC_FLAG_VEHICLE_               (GC_FLAG_RBDYMAMIC_ << 1)


class GC_Vehicle : public GC_RigidBodyDynamic
{
    DECLARE_LIST_MEMBER();
    typedef GC_RigidBodyDynamic base;
	
public:
	explicit GC_Vehicle(vec2d pos);
	explicit GC_Vehicle(FromFile);
	virtual ~GC_Vehicle();

	GC_pu_Shield* GetShield() const { return _shield; }
	void SetClass(const VehicleClass &vc); // apply vehicle class
	void SetMaxHP(float hp);
	void SetShield(GC_pu_Shield *shield) { _shield = shield; }

	float _enginePower;
	float _rotatePower;
	float _maxRotSpeed;
	float _maxLinSpeed;
	std::string _skinTextureName;

protected:
	void ApplyState(World &world, const VehicleState &vs);

public:
	VehicleState _state;

	float GetMaxSpeed() const;
	float GetMaxBrakingLength() const;

	GC_Weapon* GetWeapon() const { return _weapon; }
	void SetPlayer(World &world, GC_Player *player);
	void SetWeapon(World &world, GC_Weapon *weapon);

public:
	ObjPtr<GC_Light> _light_ambient;
	ObjPtr<GC_Light> _light1;
	ObjPtr<GC_Light> _light2;
    
	float _trackPathL;
	float _trackPathR;
	float _time_smoke;
    
	void SetSkin(const std::string &skin);
	const std::string& GetSkin() const { return _skinTextureName; }
	void SetControllerState(const VehicleState &vs);

	// GC_RigidBodyStatic
	virtual unsigned char GetPassability() const { return 0; } // not an obstacle
	virtual GC_Player* GetOwner() const { return _player; }
	
	// GC_Actor
	virtual void MoveTo(World &world, const vec2d &pos) override;

	// GC_Object
	virtual void Init(World &world);
	virtual void Kill(World &world);
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStep(World &world, float dt);
#ifdef NETWORK_DEBUG
	virtual DWORD checksum(void) const
	{
		DWORD cs = reinterpret_cast<const DWORD&>(_enginePower) ^ reinterpret_cast<const DWORD&>(_rotatePower);
		cs ^= reinterpret_cast<const DWORD&>(_maxRotSpeed) ^ reinterpret_cast<const DWORD&>(_maxLinSpeed);
		return GC_RigidBodyDynamic::checksum() ^ cs;
	}
#endif

protected:
	virtual void OnDamage(World &world, DamageDesc &dd) override;
	virtual void OnDestroy(World &world, GC_Player *by) override;
	
private:
	ObjPtr<GC_Weapon> _weapon;
	ObjPtr<GC_pu_Shield> _shield;
	ObjPtr<GC_Player> _player;

	vec2d GetLightPos1() const;
	vec2d GetLightPos2() const;
};

/////////////////////////////////////////////////////////////

class GC_Tank_Light : public GC_Vehicle
{
	DECLARE_SELF_REGISTRATION(GC_Tank_Light);

public:
	explicit GC_Tank_Light(vec2d pos);
	explicit GC_Tank_Light(FromFile);

	virtual float GetDefaultHealth() const { return 100; }
	virtual void OnDestroy(World &world, GC_Player *by) override;
};

// end of file
