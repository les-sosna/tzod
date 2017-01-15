#pragma once
#include <gc/ObjPtr.h>
#include <math/MyMath.h>
#include <list>

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

	struct PathNode
	{
		vec2d coord;
	};

	vec2d _arrivalPoint = {};
	std::list<PathNode> _path;
	AttackListType _attackList;

	float _backTime = 0;
	float _stickTime = 0;
	bool _attackFriendlyTurrets = false;

	//-------------------------------------------------------------------------
	//  to           - coordinates of the arrival point
	//  max_depth    - maximum search depth
	//  bTest        - if true then path cost is evaluated only; current path remains unchanged
	// Return: path cost or -1 if path was not found
	//-------------------------------------------------------------------------
	float CreatePath(World &world, vec2d from, vec2d to, int team, float max_depth, bool bTest, const AIWEAPSETTINGS *ws);

	// clears the current path and the attack list
	void ClearPath();

	// create additional path nodes to make it more smooth
	void SmoothPath();

	// find the nearest node to the vehicle
	std::list<PathNode>::const_iterator FindNearPathNode(const vec2d &pos, vec2d *proj, float *offset) const;

	void StayAway(const GC_Vehicle &vehicle, vec2d fromCenter, float radius);

	void ComputeState(World &world, const GC_Vehicle &vehicle, float dt, VehicleState &vs);
	void Serialize(SaveFile &f);
};

