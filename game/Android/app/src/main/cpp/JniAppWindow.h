#pragma once
#include "JniClipboard.h"
#include "JniInput.h"
#include <plat/AppWindow.h>

class JniAppWindow final : public Plat::AppWindow
{
public:
    JniAppWindow();
    ~JniAppWindow();

    Plat::AppWindowInputSink* GetInputSink() const override;
    void SetInputSink(Plat::AppWindowInputSink *inputSink) override;
    int GetDisplayRotation() const override;
    vec2d GetPixelSize() const override;
    float GetLayoutScale() const override;
    Plat::Clipboard& GetClipboard() override;
    Plat::Input& GetInput() override;
    IRender& GetRender() override;
    void SetCanNavigateBack(bool canNavigateBack) override;
    void SetMouseCursor(Plat::MouseCursor mouseCursor) override;
    void MakeCurrent() override;
    void Present() override;

private:
    Plat::AppWindowInputSink *_inputSink = nullptr;
    JniClipboard _clipboard;
    JniInput _input;
    std::unique_ptr<IRender> _render;
};
