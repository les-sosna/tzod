#pragma once
#include <plat/Input.h>

struct JniInput final
    : public Plat::Input
{
    bool IsKeyPressed(Plat::Key key) const override;
    Plat::PointerState GetPointerState(unsigned int index) const override;
    Plat::GamepadState GetGamepadState(unsigned int index) const override;
    bool GetSystemNavigationBackAvailable() const override;
};
