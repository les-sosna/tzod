#pragma once
#include <ui/UIInput.h>
#include <memory>

class StoreAppInput : public UI::IInput
{
public:
	StoreAppInput(Windows::UI::Core::CoreWindow ^coreWindow);

	// UI::IInput
	bool IsKeyPressed(UI::Key key) const override;
	bool IsMousePressed(int button) const override;
	vec2d GetMousePos() const override;

private:
	Platform::Agile<Windows::UI::Core::CoreWindow> _coreWindow;
};
