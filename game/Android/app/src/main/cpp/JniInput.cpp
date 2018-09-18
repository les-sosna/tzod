#include "JniInput.h"

bool JniInput::IsKeyPressed(UI::Key key) const
{
    return false;
}

bool JniInput::IsMousePressed(int button) const
{
    return false;
}

vec2d JniInput::GetMousePos() const
{
    return {};
}

UI::GamepadState JniInput::GetGamepadState(unsigned int index) const
{
    return {};
}

bool JniInput::GetSystemNavigationBackAvailable() const
{
    return true;
}
