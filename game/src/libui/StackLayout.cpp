#include "inc/ui/StackLayout.h"
#include <algorithm>
using namespace UI;

StackLayout::StackLayout(LayoutManager &manager)
	: Window(manager)
{
}

FRECT StackLayout::GetChildRect(vec2d size, float scale, const Window &child) const
{
	// FIXME: n^2 complexity
	float pxOffset = 0;
	float pxSpacing = std::floor(_spacing * scale);
	auto &children = GetChildren();
	if (FlowDirection::Vertical == _flowDirection)
	{
		for (auto &item : children)
		{
			if (item.get() == &child)
			{
				break;
			}
			pxOffset += pxSpacing + std::floor(item->GetHeight() * scale);
		}
		return FRECT{ 0.f, pxOffset, size.x, pxOffset + std::floor(child.GetHeight() * scale) };
	}
	else
	{
		assert(FlowDirection::Horizontal == _flowDirection);
		for (auto &item : children)
		{
			if (item.get() == &child)
			{
				break;
			}
			pxOffset += pxSpacing + std::floor(item->GetWidth() * scale);
		}
		return FRECT{ pxOffset, 0.f, pxOffset + std::floor(child.GetWidth() * scale), size.y };
	}
}
