#pragma once
#include <math/MyMath.h>

class SaveFile;
class GC_RigidBodyStatic;
class GC_Vehicle;
class World;
struct VehicleState;
struct AIWEAPSETTINGS;

class ShootingAgent final
{
public:
	void Serialize(SaveFile &f);
	void AttackTarget(World &world, const GC_Vehicle &myVehicle, const GC_RigidBodyStatic &target, float dt, VehicleState &outVehicleState);
	void SetAccuracy(int accuracy);

private:
	// for aim jitter
	float _desiredOffset = 0;
	float _currentOffset = 0;
	int _accuracy = 0;
};

