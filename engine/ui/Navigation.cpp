#include "inc/ui/LayoutContext.h"
#include "inc/ui/Navigation.h"
#include "inc/ui/Window.h"
#include "inc/ui/WindowIterator.h"
using namespace UI;

Window* UI::GetPrevFocusChild(TextureManager& texman, const LayoutContext& lc, const DataContext& dc, Window &wnd)
{
	auto focus = wnd.GetFocus();
	if (!focus && wnd.GetChildrenCount() > 0)
	{
		auto &child = wnd.GetChild(wnd.GetChildrenCount() - 1);
		LayoutContext childLC(wnd, lc, child, wnd.GetChildLayout(texman, lc, dc, child));
		if (NeedsFocus(texman, child, childLC, dc))
			return &child;
	}
	if (focus)
	{
		auto focusIt = std::find(begin(wnd), end(wnd), focus);
		while (focusIt != begin(wnd))
		{
			auto &child = **(--focusIt);
			LayoutContext childLC(wnd, lc, child, wnd.GetChildLayout(texman, lc, dc, child));
			if (NeedsFocus(texman, child, childLC, dc))
				return &child;
		}
	}
	return nullptr;
}

Window* UI::GetNextFocusChild(TextureManager& texman, const LayoutContext& lc, const DataContext& dc, Window &wnd)
{
	auto focus = wnd.GetFocus();
	if (!focus && wnd.GetChildrenCount() > 0)
	{
		auto &child = wnd.GetChild(0);
		LayoutContext childLC(wnd, lc, child, wnd.GetChildLayout(texman, lc, dc, child));
		if (NeedsFocus(texman, child, childLC, dc))
			return &child;
	}
	if (focus)
	{
		auto focusIt = std::find(rbegin(wnd), rend(wnd), focus);
		while (focusIt != rbegin(wnd))
		{
			auto &child = **(--focusIt);
			LayoutContext childLC(wnd, lc, child, wnd.GetChildLayout(texman, lc, dc, child));
			if (NeedsFocus(texman, child, childLC, dc))
				return &child;
		}
	}
	return nullptr;
}

bool UI::CanNavigateBack(TextureManager& texman, const Window& wnd, const LayoutContext& lc, const DataContext& dc)
{
	// OK to cast since CanNavigate is const
	const NavigationSink* navigationSink = const_cast<Window&>(wnd).GetNavigationSink();
	if (navigationSink && navigationSink->CanNavigate(texman, lc, dc, Navigate::Back))
		return true;
	if (auto focusedChild = wnd.GetFocus())
	{
		UI::LayoutContext childLC(wnd, lc, *focusedChild, wnd.GetChildLayout(texman, lc, dc, *focusedChild));
		return CanNavigateBack(texman, *focusedChild, childLC, dc);
	}
	return false;
}
