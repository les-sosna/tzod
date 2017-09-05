#pragma once
#include <memory>

namespace UI
{
	class Window;
	class DataContext;

	std::shared_ptr<Window> GetPrevFocusChild(const Window &wnd, const DataContext &dc);
	std::shared_ptr<Window> GetNextFocusChild(const Window &wnd, const DataContext &dc);
}
