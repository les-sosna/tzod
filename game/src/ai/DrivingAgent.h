#pragma once
#include <gc/ObjPtr.h>
#include <math/MyMath.h>
#include <list>
#include <vector>

class FieldCell;
class SaveFile;
class GC_RigidBodyStatic;
class GC_Vehicle;
class World;
struct AIWEAPSETTINGS;
struct VehicleState;

class DrivingAgent
{
public:
	typedef std::list<ObjPtr<GC_RigidBodyStatic> > AttackListType;

	//-------------------------------------------------------------------------
	//  to           - coordinates of the arrival point
	//  max_depth    - maximum search depth
	//  bTest        - if true then path cost is evaluated only; current path remains unchanged
	// Return: path cost or -1 if path was not found
	//-------------------------------------------------------------------------
	float CreatePath(World &world, vec2d from, vec2d to, int team, float max_depth, bool bTest, const AIWEAPSETTINGS *ws);
	bool HasPath() const { return !_path.empty(); }
	const std::vector<vec2d>& GetPath() const { return _path; }

	// clears the current path and the attack list
	void ClearPath();

	// create additional path nodes to make it more smooth
	void SmoothPath();

	// find the nearest node and projection to the path
	std::vector<vec2d>::const_iterator FindNearPathNode(const vec2d &pos, vec2d *proj, float *offset) const;

	void StayAway(vec2d fromCenter, float radius);

	void ComputeState(World &world, const GC_Vehicle &vehicle, float dt, VehicleState &vs);
	void Serialize(SaveFile &f);

	void SetAttackFriendlyTurrets(bool value) { _attackFriendlyTurrets = value; }

	AttackListType _attackList;
private:
	std::vector<vec2d> _path;
	int _pathProgress = -1;
	float _lastProgressTime = 0;

	vec2d _stayAwayFrom = {};
	float _stayAwayRadius = 0;

	float _backTime = 0;
	bool _attackFriendlyTurrets = false;
};

