#pragma once
#include <math/MyMath.h>

class ConfControllerProfile;
struct VehicleState;
class GC_Vehicle;
class World;
namespace UI
{
	struct IInput;
	enum class Key;
}

class Controller
{
public:
	Controller();
	void SetProfile(ConfControllerProfile &profile);
	void ReadControllerState(UI::IInput &input, World &world, const GC_Vehicle &vehicle, const vec2d *mouse, vec2d dragDirection, bool reverse, VehicleState &vs);

	void OnTap(vec2d worldPos);
	void Step(float dt);

	vec2d GetFireTarget() const { return _tapFireTarget; }
	float GetRemainingFireTime() const { return _tapFireTime; }

private:
	float _tapFireTime;
	vec2d _tapFireTarget;

	//
	// cached values from the profile
	//

	UI::Key _keyForward;
	UI::Key _keyBack;
	UI::Key _keyLeft;
	UI::Key _keyRight;
	UI::Key _keyFire;
	UI::Key _keyLight;
	UI::Key _keyTowerLeft;
	UI::Key _keyTowerRight;
	UI::Key _keyTowerCenter;
	UI::Key _keyNoPickup;
	bool _aimToMouse;
	bool _moveToMouse;
	bool _arcadeStyle;

	//
	// controller state
	//

	bool _lastLightKeyState;
	bool _lastLightsState;
};


// end of file
