#pragma once
#include <gc/Object.h>
#include <gc/ObjPtr.h>
#include <math/MyMath.h>
#include <list>

struct VehicleState;
struct AIWEAPSETTINGS;
class GC_Actor;
class GC_RigidBodyStatic;
class GC_Pickup;
class GC_Player;
class GC_Vehicle;
class SaveFile;
class World;

class DrivingAgent;
class ShootingAgent;

///////////////////////////////////////////////////////////////////////////////

typedef float AIPRIORITY;

struct AIITEMINFO
{
	ObjPtr<GC_Actor> object;
	AIPRIORITY priority;
};

class AIController
{
public:
	AIController();
	AIController(FromFile);
	virtual ~AIController();

	void Serialize(SaveFile &f);

	void OnRespawn(World &world, const GC_Vehicle &vehicle);
	void OnDie();

	void debug_draw(World &world);

	void SetLevel(int level);
	int  GetLevel() const { return _difficulty; }

	bool March(World &world, const GC_Vehicle &vehicle, float x, float y);
	bool Attack(World &world, const GC_Vehicle &vehicle, GC_RigidBodyStatic *target);
	bool Pickup(World &world, const GC_Vehicle &vehicle, GC_Pickup *p);
	void Stop();

	void ReadControllerState(World &world, float dt, const GC_Vehicle &vehicle, VehicleState &vs, bool allowExtraCalc);

private:
	// ai states
	enum aiState_l2
	{
		L2_PATH_SELECT,   // select any destination with no specific purpose
		L2_PICKUP,        // go for the item
		L2_ATTACK,        // pursue the enemy and attack if possible
	} _aiState_l2;

	// message to the top level
	enum aiState_l1
	{
		L1_NONE,
		L1_PATH_END,
	} _aiState_l1;

	void SetL1(aiState_l1 new_state);
	void SetL2(aiState_l2 new_state);

	void ProcessAction(World &world, const GC_Vehicle &vehicle, const AIWEAPSETTINGS *ws);
	void SelectState(World &world, const GC_Vehicle &vehicle, const AIWEAPSETTINGS *ws);

	void SetActive(bool active);
	bool GetActive() const { return _isActive; }

	bool IsTargetVisible(const World &world, const GC_Vehicle &vehicle, GC_RigidBodyStatic *target, GC_RigidBodyStatic** ppObstacle = nullptr);
	AIPRIORITY GetTargetRate(const GC_Vehicle &vehicle, GC_Vehicle &target);

	bool FindTarget(World &world, const GC_Vehicle &vehicle, AIITEMINFO &info, const AIWEAPSETTINGS *ws);   // return true if a target was found
	bool FindItem(World &world, const GC_Vehicle &vehicle, AIITEMINFO &info, const AIWEAPSETTINGS *ws);     // return true if something was found

	void SelectFavoriteWeapon(World &world);

	struct TargetDesc
	{
		GC_Vehicle *target;
		bool bIsVisible;
	};

	std::unique_ptr<DrivingAgent> _drivingAgent;
	std::unique_ptr<ShootingAgent> _shootingAgent;

	ObjPtr<GC_Pickup>          _pickupCurrent;
	ObjPtr<GC_RigidBodyStatic> _target;  // current target

	ObjectType _favoriteWeaponType;
	int _difficulty;
	bool _isActive;
};
