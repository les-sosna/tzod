#include "inc/ui/Window.h"
#include "inc/ui/InputContext.h"
#include "inc/ui/GuiManager.h"
#include <algorithm>

using namespace UI;

Window::Window(LayoutManager &manager)
  : _manager(manager)
  , _isVisible(true)
  , _isEnabled(true)
  , _isTopMost(false)
  , _isTimeStep(false)
  , _clipChildren(false)
{
}

Window::~Window()
{
	SetTimeStep(false);
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
	_children.push_back(std::move(child));
}

void Window::AddBack(std::shared_ptr<Window> child)
{
	_children.push_front(std::move(child));
}

FRECT Window::GetChildRect(vec2d size, float scale, const Window &child) const
{
	return CanvasLayout(child.GetOffset(), child.GetSize(), scale);
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
		OnSize(width, height);
	}
}

void Window::SetTopMost(bool topmost)
{
	_isTopMost = topmost;
}

void Window::SetTimeStep(bool enable)
{
	if( enable != _isTimeStep )
	{
		if( _isTimeStep )
			GetManager().TimeStepUnregister(_timeStepReg);
		else
			_timeStepReg = GetManager().TimeStepRegister(this);
		_isTimeStep = enable;
	}
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

void Window::SetEnabled(bool enable)
{
	_isEnabled = enable;
}

void Window::SetVisible(bool visible)
{
	_isVisible = visible;
}

const std::string& Window::GetText() const
{
	return _text;
}

void Window::SetText(TextureManager &texman, const std::string &text)
{
	_text.assign(text);
	OnTextChange(texman);
}


//
// size
//

void Window::OnSize(float width, float height)
{
}


//
// other
//

void Window::OnTextChange(TextureManager &texman)
{
}

void Window::OnTimeStep(LayoutManager &manager, float dt)
{
}


FRECT UI::CanvasLayout(vec2d offset, vec2d size, float scale)
{
	return MakeRectWH(Vec2dFloor(offset * scale), Vec2dFloor(size * scale));
}
