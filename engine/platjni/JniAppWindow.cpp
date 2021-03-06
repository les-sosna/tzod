#include "inc/platjni/JniAppWindow.h"

JniAppWindow::JniAppWindow()
{}

JniAppWindow::~JniAppWindow()
{}

void JniAppWindow::SetPixelSize(vec2d pxSize)
{
    _pxSize = pxSize;
}

int JniAppWindow::GetDisplayRotation() const
{
    return 0;
}

vec2d JniAppWindow::GetPixelSize() const
{
    return _pxSize;
}

float JniAppWindow::GetLayoutScale() const
{
    return 2;
}

Plat::Clipboard& JniAppWindow::GetClipboard()
{
    return _clipboard;
}

Plat::Input& JniAppWindow::GetInput()
{
    return _input;
}

IRender& JniAppWindow::GetRender()
{
    _render.Begin((unsigned int) _pxSize.x, (unsigned int) _pxSize.y, DO_0);
    return _render;
}

void JniAppWindow::SetCanNavigateBack(bool canNavigateBack)
{}

void JniAppWindow::SetMouseCursor(Plat::MouseCursor mouseCursor)
{}

void JniAppWindow::Present()
{
    _render.End();
}
