#include "NavStack.h"
#include <ui/DataSource.h>
#include <ui/GuiManager.h>
#include <ui/LayoutContext.h>

NavStack::NavStack(UI::TimeStepManager &manager)
	: UI::Managerful(manager)
{
}

std::shared_ptr<UI::Window> NavStack::GetNavFront() const
{
	auto &children = GetChildren();
	switch (_state)
	{
	case State::GoingForward:
		// return the staging which is being shown
		return children.empty() ? nullptr : children.back();

	case State::GoingBack:
		// skip the staging one
		return children.size() < 2 ? nullptr : children[children.size() - 2];
	}

	assert(false);
	return nullptr;
}

float NavStack::GetNavigationDepth() const
{
	float transition = GetTransitionTimeLeft() / _foldTime;
	if (_state == State::GoingForward)
		return (float)GetChildren().size() - transition;
	else
		return (float)GetChildren().size() - 1 + transition;
}

void NavStack::PopNavStack(UI::Window *wnd)
{
	auto navFront = GetNavFront();
	if (!wnd)
		wnd = navFront.get();
	assert(wnd);

	if (wnd == navFront.get())
	{
		switch (_state)
		{
		case State::GoingForward:
			// if it was going forward then just reverse the direction and keep it staging
			_state = State::GoingBack;
			_navTransitionStartTime = GetTimeStepManager().GetTime() - GetTransitionTimeLeft();
			break;

		case State::GoingBack:
			// remove the previous staging child
			UnlinkChild(*GetChildren().back());
			_navTransitionStartTime = GetTimeStepManager().GetTime();
			break;
		}
	}
	else
	{
		UnlinkChild(*wnd);
	}


	if (auto newNavFront = GetNavFront())
	{
		SetFocus(newNavFront);
	}
}

void NavStack::PushNavStack(std::shared_ptr<UI::Window> wnd)
{
	if (_state == State::GoingBack)
	{
		// remove the staging guy
		UnlinkChild(*GetChildren().back());
	}

	_state = State::GoingForward;
	_navTransitionStartTime = GetTimeStepManager().GetTime() - GetTransitionTimeLeft();

	AddFront(wnd);
	SetFocus(GetChildren().back());
}

vec2d NavStack::GetNavStackPixelSize(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc) const
{
	vec2d pxNavStackSize = {};
	if (!GetChildren().empty())
	{
		for (auto wnd : *this)
		{
			pxNavStackSize += wnd->GetContentSize(texman, dc, lc.GetScaleCombined(), DefaultLayoutConstraints(lc));
		}
		float pxSpacing = (float)(GetChildren().size() - 1) * UI::ToPx(_spacing, lc);
		pxNavStackSize += vec2d{ pxSpacing, pxSpacing };
	}
	return pxNavStackSize;
}

float NavStack::GetTransitionTimeLeft() const
{
	return std::max(0.f, _navTransitionStartTime + _foldTime - GetTimeStepManager().GetTime());
}

FRECT NavStack::GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const
{
	float transition = (1 - std::cos(PI * GetTransitionTimeLeft() / _foldTime)) / 2;

	auto &children = GetChildren();

	unsigned int dim = (_flowDirection == UI::FlowDirection::Vertical);

	float pxTotalStackSize = GetNavStackPixelSize(texman, lc, dc)[dim];
	float pxStagingSize = children.empty() ? 0 : children.back()->GetContentSize(texman, dc, lc.GetScaleCombined(), DefaultLayoutConstraints(lc))[dim];
	float pxTransitionTarget = (lc.GetPixelSize()[dim] + pxStagingSize) / 2 - pxTotalStackSize;

	const UI::Window *preStaging = children.size() > 1 ? children[children.size() - 2].get() : nullptr;
	float pxSpacing = UI::ToPx(_spacing, lc);
	float pxPreStagingSize = preStaging ? preStaging->GetContentSize(texman, dc, lc.GetScaleCombined(), DefaultLayoutConstraints(lc))[dim] : 0;
	float pxTransitionStart = (lc.GetPixelSize()[dim] + pxPreStagingSize) / 2 - (pxTotalStackSize - pxStagingSize - pxSpacing);

	if (_state == State::GoingBack)
	{
		std::swap(pxTransitionTarget, pxTransitionStart);
	}

	float pxBegin = pxTransitionStart * transition + pxTransitionTarget * (1 - transition);

	for (auto wnd : *this)
	{
		auto layoutConstraints = DefaultLayoutConstraints(lc);
		vec2d pxWndSize = wnd->GetContentSize(texman, dc, lc.GetScaleCombined(), layoutConstraints);
		pxWndSize = Vec2dMin(pxWndSize, layoutConstraints.maxPixelSize);
		if (wnd.get() == &child)
		{
			vec2d pxWndOffset = Vec2dFloor(((lc.GetPixelSize() - pxWndSize) / 2)[1 - dim], pxBegin);
			if (_flowDirection == UI::FlowDirection::Horizontal)
				pxWndOffset = Vec2dTranspose(pxWndOffset);
			return MakeRectWH(pxWndOffset,  pxWndSize);
		}
		pxBegin += pxWndSize[dim] + pxSpacing;
	}

	assert(false);

	return UI::Window::GetChildRect(texman, lc, dc, child);
}

float NavStack::GetChildOpacity(const UI::LayoutContext& lc, const UI::InputContext& ic, const Window &child) const
{
	float transition = GetTransitionTimeLeft() / _foldTime;
	if (_state == State::GoingForward)
	{
		transition = 1 - transition;
	}

	auto &children = GetChildren();
	if (children.back().get() == &child)
	{
		return transition;
	}
	else if (children[children.size() - 2].get() == &child)
	{
		return 1 - transition;
	}
	return 0;
}

bool NavStack::GetChildEnabled(const Window& child) const
{
	return GetNavFront().get() == &child;
}

void NavStack::OnPointerMove(const UI::InputContext &ic, const UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, bool captured)
{
}

bool NavStack::OnPointerDown(const UI::InputContext &ic, const UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, int button)
{
	return true;
}

void NavStack::OnPointerUp(const UI::InputContext &ic, const UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, int button)
{
}

void NavStack::OnTap(const UI::InputContext &ic, const UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition)
{
}
