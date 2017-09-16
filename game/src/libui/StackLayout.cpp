#include "inc/ui/StackLayout.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/WindowIterator.h"
#include <algorithm>
using namespace UI;

FRECT StackLayout::GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	float scale = lc.GetScale();
	vec2d size = lc.GetPixelSize();

	// FIXME: O(n^2) complexity
	float pxOffset = 0;
	float pxSpacing = std::floor(_spacing * scale);
	if (FlowDirection::Vertical == _flowDirection)
	{
		for (auto &item : *this)
		{
			if (item.get() == &child)
			{
				break;
			}
			pxOffset += pxSpacing + item->GetContentSize(texman, dc, scale, DefaultLayoutConstraints(lc)).y;
		}
		vec2d pxChildSize = child.GetContentSize(texman, dc, scale, DefaultLayoutConstraints(lc));
		if (_align == Align::LT)
		{
			return FRECT{ 0.f, pxOffset, size.x, pxOffset + pxChildSize.y };
		}
		else
		{
			assert(_align == Align::CT); // TODO: support others
			float pxMargin = std::floor(size.x - pxChildSize.x) / 2;
			return FRECT{ pxMargin, pxOffset, size.x - pxMargin, pxOffset + pxChildSize.y };
		}
	}
	else
	{
		assert(FlowDirection::Horizontal == _flowDirection);
		for (auto &item : *this)
		{
			if (item.get() == &child)
			{
				break;
			}
			pxOffset += pxSpacing + item->GetContentSize(texman, dc, scale, DefaultLayoutConstraints(lc)).x;
		}
		return FRECT{ pxOffset, 0.f, pxOffset + child.GetContentSize(texman, dc, scale, DefaultLayoutConstraints(lc)).x, size.y };
	}
}

vec2d StackLayout::GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const
{
	float pxTotalSize = 0; // in flow direction
	unsigned int sumComponent = FlowDirection::Vertical == _flowDirection;

	float pxMaxSize = 0;
	unsigned int maxComponent = FlowDirection::Horizontal == _flowDirection;

	for (auto &item : *this)
	{
		vec2d pxItemSize = item->GetContentSize(texman, dc, scale, layoutConstraints);
		pxTotalSize += pxItemSize[sumComponent];
		pxMaxSize = std::max(pxMaxSize, pxItemSize[maxComponent]);
	}

	if (GetChildrenCount() > 1)
	{
		pxTotalSize += std::floor(_spacing * scale) * (float)(GetChildrenCount() - 1);
	}

	return FlowDirection::Horizontal == _flowDirection ?
		vec2d{ pxTotalSize, pxMaxSize } :
		vec2d{ pxMaxSize, pxTotalSize };
}

std::shared_ptr<Window> StackLayout::GetNavigateTarget(const DataContext &dc, Navigate navigate)
{
	switch (navigate)
	{
	case Navigate::Prev:
		return GetPrevFocusChild(*this, dc);
	case Navigate::Next:
		return GetNextFocusChild(*this, dc);
	case Navigate::Up:
		return FlowDirection::Vertical == _flowDirection ? GetPrevFocusChild(*this, dc) : nullptr;
	case Navigate::Down:
		return FlowDirection::Vertical == _flowDirection ? GetNextFocusChild(*this, dc) : nullptr;
	case Navigate::Left:
		return FlowDirection::Horizontal == _flowDirection ? GetPrevFocusChild(*this, dc) : nullptr;
	case Navigate::Right:
		return FlowDirection::Horizontal == _flowDirection ? GetNextFocusChild(*this, dc) : nullptr;
	default:
		return nullptr;
	}
}

bool StackLayout::CanNavigate(Navigate navigate, const LayoutContext &lc, const DataContext &dc) const
{
	return !!const_cast<StackLayout*>(this)->GetNavigateTarget(dc, navigate);
}

void StackLayout::OnNavigate(Navigate navigate, NavigationPhase phase, const LayoutContext &lc, const DataContext &dc)
{
	if (NavigationPhase::Started == phase)
	{
		if (auto newFocus = GetNavigateTarget(dc, navigate))
		{
			SetFocus(std::move(newFocus));
		}
	}
}
