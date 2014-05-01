// ai.h

#pragma once

#include "gc/TypeSystem.h"
#include "gc/ObjPtr.h"
#include "core/MyMath.h"
#include <list>

// forward declarations
template<class> class JobManager;
struct VehicleState;
struct AIWEAPSETTINGS;
class FieldCell;
class GC_Actor;
class GC_RigidBodyStatic;
class GC_Pickup;
class GC_Player;
class GC_Vehicle;
class SaveFile;
class Level;


///////////////////////////////////////////////////////////////////////////////

typedef float AIPRIORITY;

struct AIITEMINFO
{
	ObjPtr<GC_Actor> object;
	AIPRIORITY priority;
};

class AIController
{
//	static JobManager<GC_PlayerAI> _jobManager;

	typedef std::list<ObjPtr<GC_RigidBodyStatic> > AttackListType;

	struct PathNode
	{
		vec2d coord;
	};


	// current path settings
	vec2d _arrivalPoint;
	std::list<PathNode> _path;
	AttackListType _attackList;


	//-------------------------------------------------------------------------
	//  to           - coordinates of the arrival point
	//  max_depth    - maximum search depth
	//  bTest        - if true then path cost is evaluated only; current path remains unchanged
	// Return: path cost or -1 if path was not found
	//-------------------------------------------------------------------------
	float CreatePath(Level &world, vec2d from, vec2d to, int team, float max_depth, bool bTest, const AIWEAPSETTINGS *ws);


	// clears the current path and the attack list
	void ClearPath();


	// create additional path nodes to make it more smooth
	void SmoothPath();


	// find the nearest node to the vehicle
	std::list<PathNode>::const_iterator FindNearPathNode(const vec2d &pos, vec2d *proj, float *offset) const;

	struct TargetDesc
	{
		GC_Vehicle *target;
        bool bIsVisible;
	};

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
		L1_STICK,
	} _aiState_l1;

protected:
	ObjPtr<GC_Pickup>          _pickupCurrent;
	ObjPtr<GC_RigidBodyStatic> _target;  // current target

	bool IsTargetVisible(Level &world, const GC_Vehicle &vehicle, GC_RigidBodyStatic *target, GC_RigidBodyStatic** ppObstacle = NULL);
	AIPRIORITY GetTargetRate(const GC_Vehicle &vehicle, GC_Vehicle &target);

	bool FindTarget(Level &world, const GC_Vehicle &vehicle, AIITEMINFO &info, const AIWEAPSETTINGS *ws);   // return true if a target was found
	bool FindItem(Level &world, const GC_Vehicle &vehicle, AIITEMINFO &info, const AIWEAPSETTINGS *ws);     // return true if something was found

	void SelectFavoriteWeapon(Level &world);

	// for aim jitter
	float _desiredOffset;
	float _currentOffset;

	ObjectType _favoriteWeaponType;

	int _level; // that is difficulty setting

	float _backTime;
	float _stickTime;

	bool _isActive;

protected:
	void TowerTo(const GC_Vehicle &vehicle, VehicleState *pState, const vec2d &x, bool bFire, const AIWEAPSETTINGS *ws);

	// calculates the position of a fake target for more accurate shooting
	// Vp - projectile speed
	void CalcOutstrip(Level &world, vec2d origin, GC_Vehicle *target, float Vp, vec2d &fake);

	void ProcessAction(Level &world, const GC_Vehicle &vehicle, const AIWEAPSETTINGS *ws);

	void SetL1(aiState_l1 new_state);
	void SetL2(aiState_l2 new_state);

	void SelectState(Level &world, const GC_Vehicle &vehicle, const AIWEAPSETTINGS *ws);
	void DoState(Level &world, const GC_Vehicle &vehicle, VehicleState *pVehState, const AIWEAPSETTINGS *ws);

	void SetActive(bool active);
	bool GetActive() const { return _isActive; }

    void Serialize(SaveFile &f);

public:
	AIController();
	AIController(FromFile);
	virtual ~AIController();

    void OnRespawn(Level &world, const GC_Vehicle &vehicle);
    void OnDie();

	void debug_draw(Level &world);

	void SetLevel(int level) { _level = level; }
	int  GetLevel() const { return _level; }

	bool March(Level &world, const GC_Vehicle &vehicle, float x, float y);
	bool Attack(Level &world, const GC_Vehicle &vehicle, GC_RigidBodyStatic *target);
	bool Pickup(Level &world, const GC_Vehicle &vehicle, GC_Pickup *p);
	void Stop();

    void ReadControllerState(Level &world, float dt, const GC_Vehicle &vehicle, VehicleState &vs);
};

///////////////////////////////////////////////////////////////////////////////
// end of file
