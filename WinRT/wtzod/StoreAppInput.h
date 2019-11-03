#pragma once
#include <plat/Input.h>
#include <memory>

class StoreAppInput final
	: public Plat::Input
{
public:
	StoreAppInput(Windows::UI::Core::CoreWindow ^coreWindow);

	// UI::IInput
	bool IsKeyPressed(Plat::Key key) const override;
	Plat::PointerState GetPointerState(unsigned int index) const override;
	Plat::GamepadState GetGamepadState(unsigned int index) const override;
	bool GetSystemNavigationBackAvailable() const override { return true; }

private:
	Platform::Agile<Windows::UI::Core::CoreWindow> _coreWindow;
};
