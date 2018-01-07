#include "inc/ui/LayoutContext.h"
#include "inc/ui/ScrollView.h"
#include <algorithm>
using namespace UI;

ScrollView::ScrollView()
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
	}
}

FRECT ScrollView::GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	float scale = lc.GetScale();
	vec2d size = lc.GetPixelSize();

	if (_content.get() == &child)
	{
		vec2d pxContentMeasuredSize = _content->GetContentSize(texman, dc, scale, DefaultLayoutConstraints(lc));
		vec2d pxContentOffset = Vec2dClamp(Vec2dFloor(_offset * scale), MakeRectWH(pxContentMeasuredSize - size));
		vec2d pxContentSize = vec2d{
			_horizontalScrollEnabled ? pxContentMeasuredSize.x : size.x,
			_verticalScrollEnabled ? pxContentMeasuredSize.y : size.y };
		return MakeRectWH(-pxContentOffset, pxContentSize);
	}

	return Window::GetChildRect(texman, lc, dc, child);
}

vec2d ScrollView::GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const
{
	return _content ? _content->GetContentSize(texman, dc, scale, layoutConstraints) : vec2d{};
}

void ScrollView::OnScroll(TextureManager &texman, const UI::InputContext &ic, const UI::LayoutContext &lc, const UI::DataContext &dc, vec2d scrollOffset)
{
	if (_content)
	{
		if (!_verticalScrollEnabled && _horizontalScrollEnabled && scrollOffset.x == 0)
		{
			std::swap(scrollOffset.x, scrollOffset.y);
		}

		vec2d pxContentMeasuredSize = _content->GetContentSize(texman, dc, lc.GetScale(), DefaultLayoutConstraints(lc));

		FRECT offsetConstraints = MakeRectWH((pxContentMeasuredSize - lc.GetPixelSize()) / lc.GetScale());
		_offset = Vec2dClamp(_offset, offsetConstraints);
		_offset -= scrollOffset * 30;
		_offset = Vec2dClamp(_offset, offsetConstraints);
	}
	else
	{
		_offset = vec2d{};
	}
}

void ScrollView::EnsureVisible(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, FRECT pxFocusRect)
{
	if (_content)
	{
		vec2d pxContentSize = _content->GetContentSize(texman, dc, lc.GetScale(), DefaultLayoutConstraints(lc));

		FRECT focusOffsetConstraints = MakeRectRB(Offset(pxFocusRect) + Size(pxFocusRect) - lc.GetPixelSize(), Offset(pxFocusRect)) / lc.GetScale();
		_offset = Vec2dClamp(_offset, focusOffsetConstraints);

		FRECT contentOffsetConstraints = MakeRectWH((pxContentSize - lc.GetPixelSize()) / lc.GetScale());
		_offset = Vec2dClamp(_offset, contentOffsetConstraints);
	}
}
