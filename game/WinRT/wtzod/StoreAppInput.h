#pragma once
#include <ui/UIInput.h>

class StoreAppInput : public UI::IInput
{
public:
	bool IsKeyPressed(UI::Key key) const override;
	bool IsMousePressed(int button) const override;
	vec2d GetMousePos() const override;
};
