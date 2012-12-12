// InputManager.h

#pragma once

#include "core/ComPtr.h"
#include "Controller.h"

#include <map>
#include <string>

class InputManager
{
public:
	InputManager(HWND hWnd);

	HRESULT InquireInputDevices();
    void ReadControllerState(const char *profile, const GC_Vehicle *vehicle, VehicleState &vs);

private:
	ComPtr<IDirectInput8> _dinput;
	ComPtr<IDirectInputDevice8> _keyboard;
//	LPDIRECTINPUTDEVICE8  g_pMouse    = NULL;

	std::map<std::string, Controller> _controllers;
	void OnProfilesChange();
};


// end of file
