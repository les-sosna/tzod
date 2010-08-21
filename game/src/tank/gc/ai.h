// ai.h: interface for the GC_PlayerAI class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "Level.h"    // FIXME!
#include "Player.h"

// forward declarations
template<class> class JobManager;
struct VehicleState;
struct AIWEAPSETTINGS;
class GC_Actor;
class GC_RigidBodyStatic;
class GC_Pickup;
class GC_Crate;

///////////////////////////////////////////////////////////////////////////////

struct AIITEMINFO
{
	SafePtr<GC_Actor> object;
	AIPRIORITY priority;
};

class GC_PlayerAI : public GC_Player
{
	DECLARE_SELF_REGISTRATION(GC_PlayerAI);

	static JobManager<GC_PlayerAI> _jobManager;

	typedef std::list<SafePtr<GC_RigidBodyStatic> > AttackListType;

protected:
	class MyPropertySet : public GC_Player::MyPropertySet
	{
		typedef GC_Player::MyPropertySet BASE;
		ObjectProperty _propLevel;
		ObjectProperty _propActive;

	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void MyExchange(bool applyToObject);
	};
	virtual PropertySet* NewPropertySet();


	struct PathNode
	{
		vec2d coord;
	};


	// current path settings
	vec2d _arrivalPoint;
	std::list<PathNode> _path;
	AttackListType _attackList;


	//-------------------------------------------------------------------------
	//  dst_x, dst_y - coordinates of the arrival point
	//  max_depth    - maximum search depth
	//  bTest        - if true then path cost is evaluated only; current path remains unchanged
	// Return: path cost or -1 if path was not found
	//-------------------------------------------------------------------------
	float CreatePath(float dst_x, float dst_y, float max_depth, bool bTest, const AIWEAPSETTINGS *ws);


	// clears the current path and the attack list
	void ClearPath();


	// create additional path nodes to make it more smooth
	void SmoothPath();


	// find the nearest node to the vehicle
	std::list<PathNode>::const_iterator FindNearPathNode(const vec2d &pos, vec2d *proj, float *offset) const;

	// check the cell's passability taking into account current weapon settings
	bool CheckCell(const FieldCell &cell) const;

	struct TargetDesc
	{
		GC_Vehicle *target;
		GC_Crate *targetBox;
		bool bIsObject;
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
	SafePtr<GC_Pickup>          _pickupCurrent;
	SafePtr<GC_RigidBodyStatic> _target;  // current target

	bool IsTargetVisible(GC_RigidBodyStatic *target, GC_RigidBodyStatic** ppObstacle = NULL);
	void LockTarget(const SafePtr<GC_RigidBodyStatic> &target);
	void FreeTarget();
	AIPRIORITY GetTargetRate(GC_Vehicle *target);
	AIPRIORITY GetTargetRateBox(GC_Crate *targetBox);

	bool FindTarget(AIITEMINFO &info, const AIWEAPSETTINGS *ws);   // return true if a target was found
	bool FindItem(AIITEMINFO &info, const AIWEAPSETTINGS *ws);     // return true if something was found

	void SelectFavoriteWeapon();

	// for aim jitter
	float _desiredOffset;
	float _currentOffset;

	ObjectType _favoriteWeaponType;

	int _level; // that is difficulty setting

	float _backTime;
	float _stickTime;

	bool _isActive;

protected:
	void RotateTo(VehicleState *pState, const vec2d &x, bool bForv, bool bBack);
	void TowerTo (VehicleState *pState, const vec2d &x, bool bFire, const AIWEAPSETTINGS *ws);

	// calculates the position of a fake target for more accurate shooting
	// Vp - projectile speed
	void CalcOutstrip(GC_Vehicle *target, float Vp, vec2d &fake);

	void ProcessAction(const AIWEAPSETTINGS *ws);

	void SetL1(aiState_l1 new_state);
	void SetL2(aiState_l2 new_state);

	void SelectState(const AIWEAPSETTINGS *ws);
	void DoState(VehicleState *pVehState, const AIWEAPSETTINGS *ws);

	void SetActive(bool active);
	bool GetActive() const { return _isActive; }

	virtual void Serialize(SaveFile &f);
	virtual void MapExchange(MapFile &f);

public:
	GC_PlayerAI();
	GC_PlayerAI(FromFile);
	virtual ~GC_PlayerAI();

	virtual void OnRespawn();
	virtual void OnDie();

	void debug_draw();

	void SetLevel(int level) { _level = level; }
	int  GetLevel() const { return _level; }

	bool March(float x, float y);
	bool Attack(GC_RigidBodyStatic *target);
	bool Pickup(GC_Pickup *p);
	void DoDefaultStats();
	void Stop();

protected:
	virtual unsigned short GetNetworkID() const { return 0; }
	virtual void TimeStepFixed(float dt);
};

///////////////////////////////////////////////////////////////////////////////
// end of file
