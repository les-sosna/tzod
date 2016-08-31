#include "inc/ui/LayoutContext.h"
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

FRECT ScrollView::GetChildRect(TextureManager &texman, const LayoutContext &lc, const StateContext &sc, const Window &child) const
{
	float scale = lc.GetScale();
	vec2d size = lc.GetPixelSize();

	if (_content.get() == &child)
	{
		vec2d pxContentMeasuredSize = _content->GetContentSize(texman, sc, scale);
		vec2d pxContentOffset = Vec2dConstrain(Vec2dFloor(_offset * scale), MakeRectWH(pxContentMeasuredSize - size));
		vec2d pxContentSize = vec2d{
			_horizontalScrollEnabled ? pxContentMeasuredSize.x : size.x,
			_verticalScrollEnabled ? pxContentMeasuredSize.y : size.y };
		return MakeRectWH(-pxContentOffset, pxContentSize);
	}

	return Window::GetChildRect(texman, lc, sc, child);
}

vec2d ScrollView::GetContentSize(TextureManager &texman, const StateContext &sc, float scale) const
{
	return _content ? _content->GetContentSize(texman, sc, scale) : vec2d{};
}

void ScrollView::OnScroll(TextureManager &texman, const UI::InputContext &ic, const UI::LayoutContext &lc, const UI::StateContext &sc, vec2d pointerPosition, vec2d scrollOffset)
{
	if (_content)
	{
		if (!_verticalScrollEnabled && _horizontalScrollEnabled && scrollOffset.x == 0)
		{
			std::swap(scrollOffset.x, scrollOffset.y);
		}

		vec2d pxContentMeasuredSize = _content->GetContentSize(texman, sc, lc.GetScale());

		FRECT offsetConstraints = MakeRectWH((pxContentMeasuredSize - lc.GetPixelSize()) / lc.GetScale());
		_offset = Vec2dConstrain(_offset, offsetConstraints);
		_offset -= scrollOffset * 30;
		_offset = Vec2dConstrain(_offset, offsetConstraints);
	}
	else
	{
		_offset = vec2d{};
	}
}
