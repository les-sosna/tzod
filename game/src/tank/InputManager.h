// InputManager.h

#pragma once

#include "core/SafePtr.h"
#include "core/ComPtr.h"

class InputManager: public RefCounted
{
public:
	InputManager(HWND hWnd);

	HRESULT InquireInputDevices();

private:
	ComPtr<IDirectInput8> _dinput;
	ComPtr<IDirectInputDevice8> _keyboard;
//	LPDIRECTINPUTDEVICE8  g_pMouse    = NULL;
};


// end of file
