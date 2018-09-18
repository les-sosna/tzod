#include "JniAppWindow.h"
#include <video/RenderOpenGL.h>

JniAppWindow::JniAppWindow()
    : _render(RenderCreateOpenGL())
{
}

JniAppWindow::~JniAppWindow()
{
}

AppWindowInputSink* JniAppWindow::GetInputSink() const
{
    return _inputSink;
}

void JniAppWindow::SetInputSink(AppWindowInputSink *inputSink)
{
    _inputSink = inputSink;
}

int JniAppWindow::GetDisplayRotation() const
{
    return 0;
}

vec2d JniAppWindow::GetPixelSize() const
{
    return vec2d{500, 500};
}

float JniAppWindow::GetLayoutScale() const
{
    return 1;
}

UI::IClipboard& JniAppWindow::GetClipboard()
{
    return _clipboard;
}

UI::IInput& JniAppWindow::GetInput()
{
    return _input;
}

IRender& JniAppWindow::GetRender()
{
    return *_render;
}

void JniAppWindow::SetCanNavigateBack(bool canNavigateBack)
{}

void JniAppWindow::SetMouseCursor(MouseCursor mouseCursor)
{}

void JniAppWindow::MakeCurrent()
{}

void JniAppWindow::Present()
{}
