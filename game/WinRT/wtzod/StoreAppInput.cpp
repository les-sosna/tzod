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

StoreAppInput::StoreAppInput(CoreWindow ^coreWindow)
	: _coreWindow(coreWindow)
{
}

bool StoreAppInput::IsKeyPressed(UI::Key key) const
{
	Windows::System::VirtualKey virtualKey = UnmapWinStoreKeyCode(key);
	if (Windows::System::VirtualKey::None != virtualKey)
	{
		return (_coreWindow->GetKeyState(virtualKey) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down;
	}
	return false;
}

bool StoreAppInput::IsMousePressed(int button) const
{
	VirtualKey virtualKey;
	switch (button)
	{
	case 1:
		virtualKey = VirtualKey::LeftButton;
		break;
	case 2:
		virtualKey = VirtualKey::RightButton;
		break;
	case 3:
		virtualKey = VirtualKey::MiddleButton;
		break;
	default:
		return false;
	}
	return (_coreWindow->GetKeyState(virtualKey) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down;
}

vec2d StoreAppInput::GetMousePos() const
{
	Point pos = _coreWindow->PointerPosition; // this can seem to throw access denied exception
	float dpi = DisplayInformation::GetForCurrentView()->LogicalDpi;
	return vec2d{ DX::ConvertDipsToPixels(pos.X - _coreWindow->Bounds.Left, dpi),
	              DX::ConvertDipsToPixels(pos.Y - _coreWindow->Bounds.Top, dpi) };
}

UI::GamepadState StoreAppInput::GetGamepadState(unsigned int index) const
{
	UI::GamepadState result = {};

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
