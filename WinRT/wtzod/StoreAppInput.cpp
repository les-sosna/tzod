#include "pch.h"
#include "StoreAppInput.h"
#include "WinStoreKeys.h"
#include "DirectXHelper.h"

using namespace Windows::Gaming::Input;
using namespace Windows::Graphics::Display;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::System;
using namespace Windows::UI::Core;

StoreAppInput::StoreAppInput(CoreWindow ^coreWindow, double scale)
	: _coreWindow(coreWindow)
	, _scale(scale)
{
}

bool StoreAppInput::IsKeyPressed(Plat::Key key) const
{
	Windows::System::VirtualKey virtualKey = UnmapWinStoreKeyCode(key);
	if (Windows::System::VirtualKey::None != virtualKey)
	{
		return (_coreWindow->GetKeyState(virtualKey) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down;
	}
	return false;
}

Plat::PointerState StoreAppInput::GetPointerState(unsigned int index) const
{
	Plat::PointerState pointerState = {};
	if (index == 0)
	{
		Point pos = _coreWindow->PointerPosition; // this can seem to throw access denied exception
		pointerState.position = vec2d{ float((pos.X - _coreWindow->Bounds.Left) * _scale),
		                               float((pos.Y - _coreWindow->Bounds.Top) * _scale) };
		pointerState.button1 = (_coreWindow->GetKeyState(VirtualKey::LeftButton) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down;
		pointerState.button2 = (_coreWindow->GetKeyState(VirtualKey::RightButton) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down;
		pointerState.button3 = (_coreWindow->GetKeyState(VirtualKey::MiddleButton) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down;
		pointerState.pressed = pointerState.button1 || pointerState.button2 || pointerState.button3;
		pointerState.type = Plat::PointerType::Mouse;
	}
	return pointerState;
}

Plat::GamepadState StoreAppInput::GetGamepadState(unsigned int index) const
{
	Plat::GamepadState result = {};

	IVectorView<Gamepad^> ^gamepads = Gamepad::Gamepads;
	if (index < gamepads->Size)
	{
		Gamepad ^gamepad = gamepads->GetAt(index);
		GamepadReading gamepadReading = gamepad->GetCurrentReading();

		result.leftTrigger = static_cast<float>(gamepadReading.LeftTrigger);
		result.rightTrigger = static_cast<float>(gamepadReading.RightTrigger);
		result.leftThumbstickPos = vec2d{ static_cast<float>(gamepadReading.LeftThumbstickX), -static_cast<float>(gamepadReading.LeftThumbstickY) };
		result.rightThumbstickPos = vec2d{ static_cast<float>(gamepadReading.RightThumbstickX), -static_cast<float>(gamepadReading.RightThumbstickY) };
		result.leftThumbstickPressed = (gamepadReading.Buttons & GamepadButtons::LeftThumbstick) != GamepadButtons::None;
		result.rightThumbstickPressed = (gamepadReading.Buttons & GamepadButtons::RightThumbstick) != GamepadButtons::None;
		result.A = (gamepadReading.Buttons & GamepadButtons::A) != GamepadButtons::None;
		result.B = (gamepadReading.Buttons & GamepadButtons::B) != GamepadButtons::None;
		result.X = (gamepadReading.Buttons & GamepadButtons::X) != GamepadButtons::None;
		result.Y = (gamepadReading.Buttons & GamepadButtons::Y) != GamepadButtons::None;
		result.leftShoulder = (gamepadReading.Buttons & GamepadButtons::LeftShoulder) != GamepadButtons::None;
		result.rightShoulder = (gamepadReading.Buttons & GamepadButtons::RightShoulder) != GamepadButtons::None;
		result.view = (gamepadReading.Buttons & GamepadButtons::View) != GamepadButtons::None;
		result.menu = (gamepadReading.Buttons & GamepadButtons::Menu) != GamepadButtons::None;
		result.DPadUp = (gamepadReading.Buttons & GamepadButtons::DPadUp) != GamepadButtons::None;
		result.DPadDown = (gamepadReading.Buttons & GamepadButtons::DPadDown) != GamepadButtons::None;
		result.DPadLeft = (gamepadReading.Buttons & GamepadButtons::DPadLeft) != GamepadButtons::None;
		result.DPadRight = (gamepadReading.Buttons & GamepadButtons::DPadRight) != GamepadButtons::None;
	}

	return result;
}
