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
    vec2d GetMousePos() const override { return vec2d(0, 0); }
};

class Clipboard : public UI::IClipboard
{
public:
    const char* GetClipboardText() const override { return nullptr; }
    void SetClipboardText(std::string text) override {}
};

@interface TouchAdapter : NSObject 
@property (nonatomic) CocoaTouchWindow* target;
- (void)handleSingleTap:(UITapGestureRecognizer *)recognizer;
@end

@implementation TouchAdapter
- (void)handleSingleTap:(UITapGestureRecognizer *)recognizer
{
    CGPoint location = [recognizer locationInView: recognizer.view];
    
    if (UI::LayoutManager *sink = self.target->GetInputSink())
    {
        sink->ProcessMouse(location.x*2, location.y*2, 0, UI::MSGLBUTTONDOWN);
        sink->ProcessMouse(location.x*2, location.y*2, 0, UI::MSGLBUTTONUP);
    }
}
@end

CocoaTouchWindow::CocoaTouchWindow(GLKView *view)
    : _glkView(view)
    , _render(RenderCreateOpenGL())
    , _inputSink(nullptr)
    , _width(110)
    , _height(110)
{
    _tapHandler = [[TouchAdapter alloc] init];
    _tapHandler.target = this;
    
    _singleFingerTap = [[UITapGestureRecognizer alloc] initWithTarget:_tapHandler
                                                               action:@selector(handleSingleTap:)];
    [view addGestureRecognizer:_singleFingerTap];
}

CocoaTouchWindow::~CocoaTouchWindow()
{
    [_glkView removeGestureRecognizer:_singleFingerTap];
}

void CocoaTouchWindow::SetPixelSize(unsigned int width, unsigned int height)
{
    if (_inputSink && (_width != width || _height != height))
    {
        _width = width;
        _height = height;
        _inputSink->GetDesktop()->Resize((float) width, (float) height);
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
    return _width;
}

unsigned int CocoaTouchWindow::GetPixelHeight()
{
    return _height;
}

void CocoaTouchWindow::SetInputSink(UI::LayoutManager *inputSink)
{
    _inputSink = inputSink;
}
