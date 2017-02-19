#include "NavStack.h"
#include <ui/DataSource.h>
#include <ui/GuiManager.h>
#include <ui/LayoutContext.h>

NavStack::NavStack(UI::LayoutManager &manager)
	: UI::Window(manager)
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
		wnd ->SetEnabled(UI::StaticValue<bool>::False());

		switch (_state)
		{
		case State::GoingForward:
			// if it was going forward then just reverse the direction and keep it staging
			_state = State::GoingBack;
			_navTransitionStartTime = GetManager().GetTime() - GetTransitionTimeLeft();
			break;

		case State::GoingBack:
			// remove the previous staging child
			UnlinkChild(*GetChildren().back());
			_navTransitionStartTime = GetManager().GetTime();
			break;
		}
	}
	else
	{
		UnlinkChild(*wnd);
	}


	if (auto newNavFront = GetNavFront())
	{
		newNavFront->SetEnabled(nullptr);
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
	_navTransitionStartTime = GetManager().GetTime() - GetTransitionTimeLeft();

	if (!GetChildren().empty())
		GetChildren().back()->SetEnabled(UI::StaticValue<bool>::False());
	AddFront(wnd);

	SetFocus(GetChildren().back());
}

float NavStack::GetNavStackPixelSize(TextureManager &texman, const UI::LayoutContext &lc, const UI::StateContext &sc) const
{
	float pxNavStackHeight = 0;
	if (!GetChildren().empty())
	{
		for (auto wnd : GetChildren())
		{
			pxNavStackHeight += wnd->GetContentSize(texman, sc, lc.GetScale()).y;
		}
		pxNavStackHeight += (float)(GetChildren().size() - 1) * UI::ToPx(_spacing, lc);
	}
	return pxNavStackHeight;
}

float NavStack::GetTransitionTimeLeft() const
{
	return std::max(0.f, _navTransitionStartTime + _foldTime - GetManager().GetTime());
}

FRECT NavStack::GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::StateContext &sc, const UI::Window &child) const
{
	float transition = (1 - std::cos(PI * GetTransitionTimeLeft() / _foldTime)) / 2;

	auto &children = GetChildren();

	float pxTotalStackHeight = GetNavStackPixelSize(texman, lc, sc);
	float pxStagingHeight = children.empty() ? 0 : children.back()->GetContentSize(texman, sc, lc.GetScale()).y;
	float pxTransitionTarget = (lc.GetPixelSize().y + pxStagingHeight) / 2 - pxTotalStackHeight;

	const UI::Window *preStaging = children.size() > 1 ? children[children.size() - 2].get() : nullptr;
	float pxSpacing = UI::ToPx(_spacing, lc);
	float pxPreStagingHeight = preStaging ? preStaging->GetContentSize(texman, sc, lc.GetScale()).y : 0;
	float pxTransitionStart = (lc.GetPixelSize().y + pxPreStagingHeight) / 2 - (pxTotalStackHeight - pxStagingHeight - pxSpacing);

	if (_state == State::GoingBack)
	{
		std::swap(pxTransitionTarget, pxTransitionStart);
	}

	float pxTop = pxTransitionStart * transition + pxTransitionTarget * (1 - transition);

	for (auto wnd : children)
	{
		vec2d pxWndSize = wnd->GetContentSize(texman, sc, lc.GetScale());
		if (wnd.get() == &child)
		{
			vec2d pxWndOffset = Vec2dFloor((lc.GetPixelSize().x - pxWndSize.x) / 2, pxTop);
			return MakeRectWH(pxWndOffset, pxWndSize);
		}
		pxTop += pxWndSize.y + pxSpacing;
	}

	assert(false);

	return UI::Window::GetChildRect(texman, lc, sc, child);
}

float NavStack::GetChildOpacity(const Window &child) const
{
	auto &children = GetChildren();
	if (!children.empty() && children.back().get() == &child)
	{
		float transition = GetTransitionTimeLeft() / _foldTime;
		if (_state == State::GoingForward)
		{
			transition = 1 - transition;
		}
		return transition;
	}
	return 1;
}

void NavStack::OnPointerMove(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, UI::PointerType pointerType, unsigned int pointerID, bool captured)
{
}

bool NavStack::OnPointerDown(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, UI::PointerType pointerType, unsigned int pointerID)
{
	return true;
}

void NavStack::OnPointerUp(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, UI::PointerType pointerType, unsigned int pointerID)
{
}

void NavStack::OnTap(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition)
{
}
