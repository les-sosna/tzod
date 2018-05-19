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
	UI::GamepadState GetGamepadState(unsigned int index) const override { return {}; }
	bool GetSystemNavigationBackAvailable() const override { return false; }
};

class Clipboard : public UI::IClipboard
{
public:
	std::string_view GetClipboardText() const override { return {}; }
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

vec2d CocoaTouchWindow::GetPixelSize() const
{
	return vec2d{ _pxWidth, _pxHeight };
}

int CocoaTouchWindow::GetDisplayRotation() const
{
	return 0;
}

float CocoaTouchWindow::GetLayoutScale() const
{
    return _scale;
}
