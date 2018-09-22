#pragma once
#include <math/MyMath.h>

class ConfControllerProfile;
class GameViewHarness;
struct VehicleState;
class GC_Vehicle;
class World;
namespace Plat
{
	struct Input;
	enum class Key;
}

class VehicleStateReader
{
public:
	VehicleStateReader();
	void SetProfile(ConfControllerProfile &profile);
	void ReadVehicleState(const GameViewHarness &gameViewHarness, const GC_Vehicle &vehicle, int playerIndex, Plat::Input &input, vec2d dragDirection, bool reverse, VehicleState &vs);

	void OnTap(vec2d worldPos);
	void Step(float dt);

	vec2d GetFireTarget() const { return _tapFireTarget; }
	float GetRemainingFireTime() const { return _tapFireTime; }

private:
	float _tapFireTime;
	vec2d _tapFireTarget;

	// cached values from the profile
	Plat::Key _keyForward;
	Plat::Key _keyBack;
	Plat::Key _keyLeft;
	Plat::Key _keyRight;
	Plat::Key _keyFire;
	Plat::Key _keyLight;
	Plat::Key _keyTowerLeft;
	Plat::Key _keyTowerRight;
	Plat::Key _keyTowerCenter;
	Plat::Key _keyNoPickup;
	int _gamepad;
	bool _aimToMouse;
	bool _moveToMouse;
	bool _arcadeStyle;

	// controller state
	bool _lastLightKeyState;
	bool _lastLightsState;
};
