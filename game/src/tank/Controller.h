// Controller.h

#pragma once

struct VehicleState;
class GC_Vehicle;
class Level;

class Controller
{
public:
	Controller();
    void SetProfile(const char *profile);
	void ReadControllerState(Level &world, const GC_Vehicle *vehicle, VehicleState &vs);


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
