#include "inc/ui/DataSource.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/InputContext.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/Window.h"
#include <algorithm>

using namespace UI;

TimeStepping::~TimeStepping()
{
	SetTimeStep(false);
}

void TimeStepping::SetTimeStep(bool enable)
{
	if (enable != _isTimeStep)
	{
		if (_isTimeStep)
			GetTimeStepManager().TimeStepUnregister(_timeStepReg);
		else
			_timeStepReg = GetTimeStepManager().TimeStepRegister(this);
		_isTimeStep = enable;
	}
}

Window::Window()
	: _isVisible(true)
	, _isTopMost(false)
	, _clipChildren(false)
{
}

void Window::Resize(float width, float height)
{
	_width  = width;
	_height = height;
}

void Window::SetTopMost(bool topmost)
{
	_isTopMost = topmost;
}

void Window::SetVisible(bool visible)
{
	_isVisible = visible;
}

///////////////////

WindowContainer::~WindowContainer()
{
	UnlinkAllChildren();
}

void WindowContainer::UnlinkAllChildren()
{
	_focusChild = nullptr;
	_children.clear();
}

void WindowContainer::UnlinkChild(Window& child)
{
	if (_focusChild == &child)
		_focusChild = nullptr;
	_children.erase(
		std::remove_if(std::begin(_children), std::end(_children), [&](auto & which) { return which.get() == &child; }),
		_children.end());
}

void WindowContainer::AddFront(std::shared_ptr<Window> child)
{
	assert(child);
	_children.push_back(std::move(child));
}

void WindowContainer::AddBack(std::shared_ptr<Window> child)
{
	assert(child);
	_children.push_front(std::move(child));
}

unsigned int WindowContainer::GetChildrenCount() const
{
	return static_cast<unsigned int>(_children.size());
}

std::shared_ptr<const Window> WindowContainer::GetChild(const std::shared_ptr<const Window>& owner, unsigned int index) const
{
	return _children[index];
}

const Window& WindowContainer::GetChild(unsigned int index) const
{
	return *_children[index];
}

void WindowContainer::SetFocus(Window* child)
{
	assert(!child || end(_children) != std::find_if(begin(_children), end(_children), [=](auto & c) { return c.get() == child; }));
	_focusChild = child;
}

std::shared_ptr<const Window> WindowContainer::GetFocus(const std::shared_ptr<const Window> & owner) const
{
	return _focusChild ? *std::find_if(begin(_children), end(_children), [=](auto & c) { return c.get() == _focusChild; }) : nullptr;
}

const Window* WindowContainer::GetFocus() const
{
	return _focusChild;
}


// Utils

bool UI::NeedsFocus(TextureManager& texman, const InputContext& ic, const Window& wnd, const LayoutContext& lc, const DataContext& dc)
{
	if (!wnd.GetVisible())
		return false;

	if (wnd.HasNavigationSink() || wnd.HasKeyboardSink() || wnd.HasTextSink())
		return true;

	if (auto focus = wnd.GetFocus())
	{
		auto childLayout = wnd.GetChildLayout(texman, lc, dc, *focus);
		if (childLayout.enabled && NeedsFocus(texman, ic, *focus, LayoutContext(ic, wnd, lc, *focus, childLayout), dc))
			return true;
	}

	return false;
}

FRECT UI::CanvasLayout(vec2d offset, vec2d size, float scale)
{
	return MakeRectWH(ToPx(offset, scale), ToPx(size, scale));
}
