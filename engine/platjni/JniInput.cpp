#include "inc/platjni/JniInput.h"

bool JniInput::IsKeyPressed(Plat::Key key) const
{
    return false;
}

Plat::PointerState JniInput::GetPointerState(unsigned int index) const
{
    return {};
}

Plat::GamepadState JniInput::GetGamepadState(unsigned int index) const
{
    return {};
}

bool JniInput::GetSystemNavigationBackAvailable() const
{
    return true;
}
