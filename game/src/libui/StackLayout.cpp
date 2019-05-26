#include "inc/ui/StackLayout.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/WindowIterator.h"
#include <algorithm>
using namespace UI;

#include "inc/ui/Button.h"

FRECT StackLayout::GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	float scale = lc.GetScaleCombined();
	vec2d size = lc.GetPixelSize();

	// FIXME: O(n^2) complexity
	float pxOffset = 0;
	float pxSpacing = std::floor(_spacing * scale);
	LayoutConstraints constraints = DefaultLayoutConstraints(lc);
	if (FlowDirection::Vertical == _flowDirection)
	{
		for (auto item : *this)
		{
			if (item.get() == &child)
			{
				break;
			}

			float pxSpaceTaken = item->GetContentSize(texman, dc, scale, constraints).y + pxSpacing;
			pxOffset += pxSpaceTaken;
			constraints.maxPixelSize.y = std::max(constraints.maxPixelSize.y - pxSpaceTaken, 0.f);
		}
		vec2d pxChildSize = Vec2dMin(child.GetContentSize(texman, dc, scale, constraints), constraints.maxPixelSize);
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
		for (auto item : *this)
		{
			if (item.get() == &child)
			{
				break;
			}
			float pxSpaceTaken = item->GetContentSize(texman, dc, scale, constraints).x + pxSpacing;
			constraints.maxPixelSize.x = std::max(constraints.maxPixelSize.x - pxSpaceTaken, 0.f);
			pxOffset += pxSpaceTaken;
		}
		vec2d pxChildSize = Vec2dMin(child.GetContentSize(texman, dc, scale, constraints), constraints.maxPixelSize);
		return FRECT{ pxOffset, 0.f, pxOffset + pxChildSize.x, size.y };
	}
}

vec2d StackLayout::GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const
{
	assert(GetWidth() == 0 && GetHeight() == 0); // explicit size is ignored

	float pxTotalSize = 0; // in flow direction
	unsigned int sumComponent = FlowDirection::Vertical == _flowDirection;

	float pxMaxSize = 0;
	unsigned int maxComponent = FlowDirection::Horizontal == _flowDirection;

	for (auto item: *this)
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

std::shared_ptr<Window> StackLayout::GetNavigateTarget(Navigate navigate)
{
	switch (navigate)
	{
	case Navigate::Prev:
		return GetPrevFocusChild(*this);
	case Navigate::Next:
		return GetNextFocusChild(*this);
	case Navigate::Up:
		return FlowDirection::Vertical == _flowDirection ? GetPrevFocusChild(*this) : nullptr;
	case Navigate::Down:
		return FlowDirection::Vertical == _flowDirection ? GetNextFocusChild(*this) : nullptr;
	case Navigate::Left:
		return FlowDirection::Horizontal == _flowDirection ? GetPrevFocusChild(*this) : nullptr;
	case Navigate::Right:
		return FlowDirection::Horizontal == _flowDirection ? GetNextFocusChild(*this) : nullptr;
	default:
		return nullptr;
	}
}

bool StackLayout::CanNavigate(Navigate navigate, const LayoutContext &lc) const
{
	return !!const_cast<StackLayout*>(this)->GetNavigateTarget(navigate);
}

void StackLayout::OnNavigate(Navigate navigate, NavigationPhase phase, const LayoutContext &lc)
{
	if (NavigationPhase::Started == phase)
	{
		if (auto newFocus = GetNavigateTarget(navigate))
		{
			SetFocus(std::move(newFocus));
		}
	}
}
