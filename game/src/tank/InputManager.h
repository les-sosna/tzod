// InputManager.h

#pragma once

#include "Controller.h"

#include <map>
#include <string>

class InputManager
{
public:
	InputManager();
    ~InputManager();

    void ReadControllerState(const char *profile, const GC_Vehicle *vehicle, VehicleState &vs);

private:
	std::map<std::string, Controller> _controllers;
	void OnProfilesChange();
};

// end of file
