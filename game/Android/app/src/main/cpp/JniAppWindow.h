#pragma once
#include "JniClipboard.h"
#include "JniInput.h"
#include <plat/AppWindow.h>

class JniAppWindow final : public AppWindow
{
    AppWindowInputSink *_inputSink = nullptr;
    JniClipboard _clipboard;
    JniInput _input;
    std::unique_ptr<IRender> _render;
public:
    JniAppWindow();
    ~JniAppWindow();

    AppWindowInputSink* GetInputSink() const override;
    void SetInputSink(AppWindowInputSink *inputSink) override;
    int GetDisplayRotation() const override;
    vec2d GetPixelSize() const override;
    float GetLayoutScale() const override;
    UI::IClipboard& GetClipboard() override;
    UI::IInput& GetInput() override;
    IRender& GetRender() override;
    void SetCanNavigateBack(bool canNavigateBack) override;
    void SetMouseCursor(MouseCursor mouseCursor) override;
    void MakeCurrent() override;
    void Present() override;
};
