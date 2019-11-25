#import "CocoaTouchWindow.h"
#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>
#include <plat/Input.h>
#include <plat/Clipboard.h>
#include <video/RenderOpenGL.h>

class Input : public Plat::Input
{
public:
    // Plat::Input
    bool IsKeyPressed(Plat::Key key) const override { return false; }
    Plat::PointerState GetPointerState(unsigned int index) const override { return {}; }
    Plat::GamepadState GetGamepadState(unsigned int index) const override { return {}; }
	bool GetSystemNavigationBackAvailable() const override { return false; }
};

class Clipboard : public Plat::Clipboard
{
public:
	std::string_view GetClipboardText() const override { return {}; }
    void SetClipboardText(std::string text) override {}
};

CocoaTouchWindow::CocoaTouchWindow()
    : _render(RenderCreateOpenGL())
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

Plat::Clipboard& CocoaTouchWindow::GetClipboard()
{
    static Clipboard clipboard;
    return clipboard;
}

Plat::Input& CocoaTouchWindow::GetInput()
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
