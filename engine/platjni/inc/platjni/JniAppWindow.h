#pragma once
#include "JniClipboard.h"
#include "JniInput.h"
#include <plat/AppWindow.h>

class JniAppWindow final : public Plat::AppWindow
{
public:
    JniAppWindow();
    ~JniAppWindow();

    void SetPixelSize(vec2d pxSize);

    // Plat::AppWindow
    int GetDisplayRotation() const override;
    vec2d GetPixelSize() const override;
    float GetLayoutScale() const override;
    Plat::Clipboard& GetClipboard() override;
    Plat::Input& GetInput() override;
    IRender& GetRender() override;
    void SetCanNavigateBack(bool canNavigateBack) override;
    void SetMouseCursor(Plat::MouseCursor mouseCursor) override;
    void Present() override;

private:
    JniClipboard _clipboard;
    JniInput _input;
    std::unique_ptr<IRender> _render;
    vec2d _pxSize = {};
};
