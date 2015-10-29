#pragma once

class ConfControllerProfile;
struct VehicleState;
class GC_Vehicle;
class World;
class vec2d;
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
	void ReadControllerState(UI::IInput &input, World &world, const GC_Vehicle *vehicle, const vec2d *mouse, VehicleState &vs);

private:
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
	UI::Key _keyPickup;
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
