#pragma once
#include <memory>

namespace UI
{
	class Window;
	class LayoutContext;

	enum class Navigate
	{
		None,
		Enter,
		Back,
		Prev,
		Next,
		Up,
		Down,
		Left,
		Right,
		Begin,
		End,
		Menu,
	};

	enum class NavigationPhase
	{
		Started,
		Completed,
		Aborted,
	};

	struct NavigationSink
	{
		virtual bool CanNavigate(Navigate navigate, const LayoutContext &lc) const = 0;
		virtual void OnNavigate(Navigate navigate, NavigationPhase phase, const LayoutContext &lc) = 0;
	};

	std::shared_ptr<Window> GetPrevFocusChild(Window &wnd);
	std::shared_ptr<Window> GetNextFocusChild(Window &wnd);
}
