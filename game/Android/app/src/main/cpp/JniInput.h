#pragma once
#include <ui/UIInput.h>

struct JniInput final : public UI::IInput
{
    bool IsKeyPressed(UI::Key key) const override;
    bool IsMousePressed(int button) const override;
    vec2d GetMousePos() const override;
    UI::GamepadState GetGamepadState(unsigned int index) const override;
    bool GetSystemNavigationBackAvailable() const override;
};
