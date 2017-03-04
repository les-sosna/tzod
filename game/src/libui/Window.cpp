#include "inc/ui/DataSource.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/InputContext.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/Window.h"
#include <algorithm>

using namespace UI;

Managerful::~Managerful()
{
	SetTimeStep(false);
}

void Managerful::SetTimeStep(bool enable)
{
	if (enable != _isTimeStep)
	{
		if (_isTimeStep)
			GetManager().TimeStepUnregister(_timeStepReg);
		else
			_timeStepReg = GetManager().TimeStepRegister(this);
		_isTimeStep = enable;
	}
}

Window::Window()
  : _isVisible(true)
  , _isTopMost(false)
  , _clipChildren(false)
{
}

Window::~Window()
{
	UnlinkAllChildren();
}

void Window::UnlinkAllChildren()
{
	_focusChild.reset();
	_children.clear();
}

void Window::UnlinkChild(Window &child)
{
	if (_focusChild.get() == &child)
		_focusChild = nullptr;
	_children.erase(
		std::remove_if(std::begin(_children), std::end(_children), [&](auto &which) { return which.get() == &child;} ),
		_children.end());
}

void Window::AddFront(std::shared_ptr<Window> child)
{
	assert(child);
	_children.push_back(std::move(child));
}

void Window::AddBack(std::shared_ptr<Window> child)
{
	_children.push_front(std::move(child));
}

FRECT Window::GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	return CanvasLayout(child.GetOffset(), child.GetSize(), lc.GetScale());
}

void Window::Move(float x, float y)
{
	_x = x;
	_y = y;
}

void Window::Resize(float width, float height)
{
	if( _width != width || _height != height )
	{
		_width  = width;
		_height = height;
	}
}

void Window::SetTopMost(bool topmost)
{
	_isTopMost = topmost;
}

void Window::SetFocus(std::shared_ptr<Window> child)
{
	assert(!child || _children.end() != std::find(_children.begin(), _children.end(), child));
	_focusChild = child;
}

std::shared_ptr<Window> Window::GetFocus() const
{
	return _focusChild;
}

void Window::SetEnabled(std::shared_ptr<LayoutData<bool>> enabled)
{
	_enabled = std::move(enabled);
}

bool Window::GetEnabled(const DataContext &dc) const
{
	return _enabled ? _enabled->GetValue(dc) : true;
}

void Window::SetVisible(bool visible)
{
	_isVisible = visible;
}


FRECT UI::CanvasLayout(vec2d offset, vec2d size, float scale)
{
	return MakeRectWH(ToPx(offset, scale), ToPx(size, scale));
}
