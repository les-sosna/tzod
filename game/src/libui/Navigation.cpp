#include "inc/ui/Navigation.h"
#include "inc/ui/Window.h"
using namespace UI;

std::shared_ptr<Window> UI::GetPrevFocusChild(const Window &wnd, const DataContext &dc)
{
	auto &children = wnd.GetChildren();
	auto focusChild = wnd.GetFocus();
	if (!focusChild && !children.empty())
	{
		focusChild = children.back();
		if (NeedsFocus(focusChild.get(), dc))
			return focusChild;
	}
	if (focusChild)
	{
		auto focusIt = std::find(children.begin(), children.end(), focusChild);
		while (focusIt != children.begin())
		{
			focusIt--;
			if (NeedsFocus(focusIt->get(), dc))
			{
				return *focusIt;
			}
		}
	}
	return nullptr;
}

std::shared_ptr<Window> UI::GetNextFocusChild(const Window &wnd, const DataContext &dc)
{
	auto &children = wnd.GetChildren();
	auto focusChild = wnd.GetFocus();
	if (!focusChild && !children.empty())
	{
		focusChild = children.front();
		if (NeedsFocus(focusChild.get(), dc))
			return focusChild;
	}
	if (focusChild)
	{
		auto focusIt = std::find(children.rbegin(), children.rend(), focusChild);
		while (focusIt != children.rbegin())
		{
			focusIt--;
			if (NeedsFocus(focusIt->get(), dc))
			{
				return *focusIt;
			}
		}
	}
	return nullptr;
}
