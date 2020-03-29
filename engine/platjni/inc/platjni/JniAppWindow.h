#pragma once
#include "JniClipboard.h"
#include "JniInput.h"
#include <plat/AppWindow.h>
#include <video/RenderGLES2.h>
#include <video/RenderBinding.h>

class JniAppWindow final : public Plat::AppWindow
{
public:
    JniAppWindow();
    ~JniAppWindow();

    void SetPixelSize(vec2d pxSize);
    IRender& GetRender();
    RenderBinding& GetRenderBinding() { return _renderBinding; }
    void Present();

    // Plat::AppWindow
    int GetDisplayRotation() const override;
    vec2d GetPixelSize() const override;
    float GetLayoutScale() const override;
    Plat::Clipboard& GetClipboard() override;
    Plat::Input& GetInput() override;
    void SetCanNavigateBack(bool canNavigateBack) override;
    void SetMouseCursor(Plat::MouseCursor mouseCursor) override;

private:
    JniClipboard _clipboard;
    JniInput _input;
    RenderGLES2 _render;
    RenderBinding _renderBinding;
    vec2d _pxSize = {};
};
