#import "CocoaTouchWindow.h"
#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>
#include <video/RenderOpenGL.h>
#include <ui/UIInput.h>
#include <ui/Clipboard.h>
#include <ui/GuiManager.h>
#include <ui/Window.h>

class Input : public UI::IInput
{
public:
    // UI::IInput
    bool IsKeyPressed(UI::Key key) const override { return false; }
    bool IsMousePressed(int button) const override { return false; }
    vec2d GetMousePos() const override { return vec2d(); }
};

class Clipboard : public UI::IClipboard
{
public:
    const char* GetClipboardText() const override { return nullptr; }
    void SetClipboardText(std::string text) override {}
};

CocoaTouchWindow::CocoaTouchWindow(GLKView *view)
    : _glkView(view)
    , _render(RenderCreateOpenGL())
{
}

CocoaTouchWindow::~CocoaTouchWindow()
{
}

void CocoaTouchWindow::SetSizeAndScale(float width, float height, float scale)
{
    float newPxWidth = width * scale;
    float newPxHeight = height * scale;
    
    if (_pxWidth != newPxWidth || _pxHeight != newPxHeight || _scale != scale)
    {
        _pxWidth = newPxWidth;
        _pxHeight = newPxHeight;
        _scale = scale;
        if (_inputSink)
        {
            _inputSink->GetDesktop()->Resize(GetPixelWidth() / GetLayoutScale(), GetPixelHeight() / GetLayoutScale());
        }
    }
}

UI::IClipboard& CocoaTouchWindow::GetClipboard()
{
    static Clipboard clipboard;
    return clipboard;
}

UI::IInput& CocoaTouchWindow::GetInput()
{
    static Input input;
    return input;
}

IRender& CocoaTouchWindow::GetRender()
{
    return *_render;
}

float CocoaTouchWindow::GetPixelWidth() const
{
    return _pxWidth;
}

float CocoaTouchWindow::GetPixelHeight() const
{
    return _pxHeight;
}

float CocoaTouchWindow::GetLayoutScale() const
{
    return _scale;
}

void CocoaTouchWindow::SetInputSink(UI::LayoutManager *inputSink)
{
    _inputSink = inputSink;
    if (_inputSink)
    {
        _inputSink->GetDesktop()->Resize(GetPixelWidth() / GetLayoutScale(), GetPixelHeight() / GetLayoutScale());
    }
}
