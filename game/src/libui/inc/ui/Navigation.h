#pragma once
#include <memory>

class TextureManager;

namespace UI
{
	class DataContext;
	class InputContext;
	class LayoutContext;
	class Window;

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
		virtual bool CanNavigate(TextureManager& texman, const InputContext &ic, const LayoutContext& lc, const DataContext& dc, Navigate navigate) const = 0;
		virtual void OnNavigate(TextureManager& texman, const InputContext &ic, const LayoutContext& lc, const DataContext& dc, Navigate navigate, NavigationPhase phase) = 0;
	};

	Window* GetPrevFocusChild(TextureManager& texman, const InputContext& ic, const LayoutContext& lc, const DataContext& dc, Window& wnd);
	Window* GetNextFocusChild(TextureManager& texman, const InputContext& ic, const LayoutContext& lc, const DataContext& dc, Window& wnd);
}
