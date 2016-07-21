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
    if (_inputSink && (_width != width || _height != height || _scale != scale))
    {
        _width = width;
        _height = height;
        _scale = scale;
        _inputSink->GetDesktop()->Resize(width, height);
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

unsigned int CocoaTouchWindow::GetPixelWidth()
{
    return static_cast<unsigned int>(_width * _scale);
}

unsigned int CocoaTouchWindow::GetPixelHeight()
{
    return static_cast<unsigned int>(_height * _scale);
}

float CocoaTouchWindow::GetLayoutScale()
{
    return _scale;
}

void CocoaTouchWindow::SetInputSink(UI::LayoutManager *inputSink)
{
    _inputSink = inputSink;
}
