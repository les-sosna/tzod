#include "inc/ui/Navigation.h"
#include "inc/ui/Window.h"
#include "inc/ui/WindowIterator.h"
using namespace UI;

std::shared_ptr<Window> UI::GetPrevFocusChild(Window &wnd)
{
	auto focusChild = wnd.GetFocus();
	if (!focusChild && wnd.GetChildrenCount() > 0)
	{
		focusChild = wnd.GetChild(wnd.GetChildrenCount() - 1);
		if (NeedsFocus(focusChild.get()))
			return focusChild;
	}
	if (focusChild)
	{
		auto focusIt = std::find(begin(wnd), end(wnd), focusChild);
		while (focusIt != begin(wnd))
		{
			--focusIt;
			if (NeedsFocus((*focusIt).get()))
			{
				return *focusIt;
			}
		}
	}
	return nullptr;
}

std::shared_ptr<Window> UI::GetNextFocusChild(Window &wnd)
{
	auto focusChild = wnd.GetFocus();
	if (!focusChild && wnd.GetChildrenCount() > 0)
	{
		focusChild = wnd.GetChild(0);
		if (NeedsFocus(focusChild.get()))
			return focusChild;
	}
	if (focusChild)
	{
		auto focusIt = std::find(rbegin(wnd), rend(wnd), focusChild);
		while (focusIt != rbegin(wnd))
		{
			--focusIt;
			if (NeedsFocus((*focusIt).get()))
			{
				return *focusIt;
			}
		}
	}
	return nullptr;
}
