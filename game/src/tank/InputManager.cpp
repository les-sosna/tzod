// InputManager.cpp

#include "stdafx.h"
#include "InputManager.h"

#include "core/Console.h"
#include "core/debug.h"

InputManager::InputManager(HWND hWnd)
{
	TRACE("init direct input\n");

	ZeroMemory(g_env.envInputs.keys, sizeof(g_env.envInputs.keys));

	DWORD dwPriority = DISCL_NONEXCLUSIVE | DISCL_FOREGROUND;


    HRESULT hr;

    if( FAILED(hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (VOID**) &_dinput, NULL)) )
		throw std::runtime_error("DirectInput8Create failed");
    if( FAILED(hr = _dinput->CreateDevice(GUID_SysKeyboard, &_keyboard, NULL)) )
		throw std::runtime_error("IDirectInput8::CreateDevice failed");
    if( FAILED(hr = _keyboard->SetDataFormat(&c_dfDIKeyboard)) )
        throw std::runtime_error("IDirectInputDevice8::SetDataFormat failed");
    if( FAILED(hr = _keyboard->SetCooperativeLevel(hWnd, dwPriority)) )
        throw std::runtime_error("IDirectInputDevice8::SetCooperativeLevel failed");
    _keyboard->Acquire();

//    if( FAILED( hr = g_pDI->CreateDevice( GUID_SysMouse, &g_pMouse, NULL ) ) )
//        return hr;
//    if( FAILED( hr = g_pMouse->SetDataFormat( &c_dfDIMouse2 ) ) )
//        return hr;
//    if( FAILED(hr = g_pMouse->SetCooperativeLevel( hWnd, dwPriority)) )
//        return hr;
//    g_pMouse->Acquire();

	g_env.envInputs.mouse_x = 0;//g_render->GetWidth() / 2;
	g_env.envInputs.mouse_y = 0;//g_render->GetHeight() / 2;
	g_env.envInputs.mouse_wheel = 0;
	g_env.envInputs.bLButtonState = false;
	g_env.envInputs.bRButtonState = false;
	g_env.envInputs.bMButtonState = false;
}


HRESULT InputManager::InquireInputDevices()
{
	HRESULT hr;

	if( !_keyboard )
		return S_OK;

	char data[256];
	if( FAILED(hr = _keyboard->GetDeviceState(sizeof(data), data)) )
	{
		hr = _keyboard->Acquire();
		while( DIERR_INPUTLOST == hr )
		{
			Sleep(50);
			hr = _keyboard->Acquire();
		}
		return S_OK;
	}

	ZeroMemory(g_env.envInputs.keys, sizeof(g_env.envInputs.keys));
	for( int i = 0; i < sizeof(data); ++i )
	{
		g_env.envInputs.keys[i] = (data[i] & 0x80) != 0;
	}


/*
    DIMOUSESTATE2 dims2 = {0};

    hr = g_pMouse->GetDeviceState( sizeof(DIMOUSESTATE2), &dims2 );
    if( FAILED(hr) )
    {
        hr = g_pMouse->Acquire();
        while( hr == DIERR_INPUTLOST )
		{
			Sleep(50);
            hr = g_pMouse->Acquire();
		}
        return S_OK;
    }

	g_env.envInputs.mouse_wheel = dims2.lZ;

	g_env.envInputs.mouse_x += dims2.lX;
	g_env.envInputs.mouse_y += dims2.lY;
	g_env.envInputs.mouse_x = __max(0,
		__min(g_render->GetWidth() - 1, g_env.envInputs.mouse_x));
	g_env.envInputs.mouse_y = __max(0,
		__min(g_render->GetHeight() - 1, g_env.envInputs.mouse_y));


	g_env.envInputs.bLButtonState = (dims2.rgbButtons[0] & 0x80) != 0;
	g_env.envInputs.bRButtonState = (dims2.rgbButtons[1] & 0x80) != 0;
	g_env.envInputs.bMButtonState = (dims2.rgbButtons[2] & 0x80) != 0;
*/

    return S_OK;
}


// end of file
