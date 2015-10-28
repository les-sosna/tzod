// Controller.h

#pragma once

class ConfControllerProfile;
struct VehicleState;
class GC_Vehicle;
class World;
class vec2d;
namespace UI
{
	struct IInput;
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

	int _keyForward;
	int _keyBack;
	int _keyLeft;
	int _keyRight;
	int _keyFire;
	int _keyLight;
	int _keyTowerLeft;
	int _keyTowerRight;
	int _keyTowerCenter;
	int _keyPickup;
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
