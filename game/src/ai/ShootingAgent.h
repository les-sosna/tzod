#pragma once
#include <math/MyMath.h>

class SaveFile;
class GC_RigidBodyStatic;
class GC_Vehicle;
class World;
struct VehicleState;
struct AIWEAPSETTINGS;

class ShootingAgent
{
public:
	// for aim jitter
	float _desiredOffset = 0;
	float _currentOffset = 0;

	void TowerTo(const GC_Vehicle &vehicle, VehicleState *pState, const vec2d &x, bool bFire, const AIWEAPSETTINGS *ws);

	// calculates the position of a fake target for more accurate shooting
	// Vp - projectile speed
	void CalcOutstrip(const World &world, vec2d origin, const GC_Vehicle &target, float Vp, vec2d &fake);

	void Serialize(SaveFile &f);
	void AttackTarget(World &world, const GC_Vehicle &myVehicle, const GC_RigidBodyStatic &target, VehicleState *pVehState);
};

