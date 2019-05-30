#include "inc/ui/LayoutContext.h"
#include "inc/ui/Navigation.h"
#include "inc/ui/Window.h"
#include "inc/ui/WindowIterator.h"
using namespace UI;

Window* UI::GetPrevFocusChild(TextureManager& texman, const InputContext &ic, const LayoutContext& lc, const DataContext& dc, Window &wnd)
{
	auto focus = wnd.GetFocus();
	if (!focus && wnd.GetChildrenCount() > 0)
	{
		auto &child = wnd.GetChild(wnd.GetChildrenCount() - 1);
		LayoutContext childLC(ic, wnd, lc, child, wnd.GetChildLayout(texman, lc, dc, child));
		if (NeedsFocus(texman, ic, child, childLC, dc))
			return &child;
	}
	if (focus)
	{
		auto focusIt = std::find(begin(wnd), end(wnd), focus);
		while (focusIt != begin(wnd))
		{
			auto &child = **(--focusIt);
			LayoutContext childLC(ic, wnd, lc, child, wnd.GetChildLayout(texman, lc, dc, child));
			if (NeedsFocus(texman, ic, child, childLC, dc))
				return &child;
		}
	}
	return nullptr;
}

Window* UI::GetNextFocusChild(TextureManager& texman, const InputContext& ic, const LayoutContext& lc, const DataContext& dc, Window &wnd)
{
	auto focus = wnd.GetFocus();
	if (!focus && wnd.GetChildrenCount() > 0)
	{
		auto &child = wnd.GetChild(0);
		LayoutContext childLC(ic, wnd, lc, child, wnd.GetChildLayout(texman, lc, dc, child));
		if (NeedsFocus(texman, ic, child, childLC, dc))
			return &child;
	}
	if (focus)
	{
		auto focusIt = std::find(rbegin(wnd), rend(wnd), focus);
		while (focusIt != rbegin(wnd))
		{
			auto &child = **(--focusIt);
			LayoutContext childLC(ic, wnd, lc, child, wnd.GetChildLayout(texman, lc, dc, child));
			if (NeedsFocus(texman, ic, child, childLC, dc))
				return &child;
		}
	}
	return nullptr;
}
