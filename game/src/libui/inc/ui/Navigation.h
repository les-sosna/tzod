#pragma once
#include <memory>

namespace UI
{
	class Window;
	class DataContext;

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
	};

	enum class NavigationPhase
	{
		Started,
		Completed,
		Aborted,
	};

	struct NavigationSink
	{
		virtual bool CanNavigate(Navigate navigate, const DataContext &dc) const = 0;
		virtual void OnNavigate(Navigate navigate, NavigationPhase phase, const DataContext &dc) = 0;
	};

	std::shared_ptr<Window> GetPrevFocusChild(const Window &wnd, const DataContext &dc);
	std::shared_ptr<Window> GetNextFocusChild(const Window &wnd, const DataContext &dc);
}
