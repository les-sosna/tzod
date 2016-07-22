#include "inc/ui/ScrollView.h"
#include <algorithm>
using namespace UI;

ScrollView::ScrollView(LayoutManager &manager)
	: Window(manager)
{
	SetClipChildren(true);
}

void ScrollView::SetContent(std::shared_ptr<Window> content)
{
	if (_content != content)
	{
		if (_content)
		{
			UnlinkChild(*_content);
			_offset = vec2d{};
		}
		_content = content;
		AddBack(_content);
		SetFocus(_content);
	}
}

FRECT ScrollView::GetChildRect(vec2d size, float scale, const Window &child) const
{
	if (_content.get() == &child)
	{
		return CanvasLayout(-_offset, vec2d{ size.x / scale, child.GetSize().y }, scale);
	}

	return Window::GetChildRect(size, scale, child);
}

void ScrollView::OnScroll(UI::InputContext &ic, vec2d size, float scale, vec2d pointerPosition, vec2d offset)
{
	if (_content)
	{
		_offset -= offset * 30;
		_offset.y = std::max(0.f, std::min(_content->GetHeight() - size.y / scale, _offset.y));
		_offset.x = std::max(0.f, std::min(size.x / scale - _content->GetWidth(), _offset.x));
	}
	else
	{
		_offset = vec2d{};
	}
}
