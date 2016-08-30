#include "inc/ui/StackLayout.h"
#include "inc/ui/LayoutContext.h"
#include <algorithm>
using namespace UI;

StackLayout::StackLayout(LayoutManager &manager)
	: Window(manager)
{
}

FRECT StackLayout::GetChildRect(TextureManager &texman, const LayoutContext &lc, const StateContext &sc, const Window &child) const
{
	float scale = lc.GetScale();
	vec2d size = lc.GetPixelSize();

	// FIXME: O(n^2) complexity
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

float StackLayout::GetWidth() const
{
	if (FlowDirection::Horizontal == _flowDirection)
	{
		float totalWidth = 0;
		auto &children = GetChildren();
		for (auto &item : children)
		{
			totalWidth += item->GetWidth();
		}
		if (children.size() > 1)
		{
			totalWidth += (float)(children.size() - 1) * _spacing;
		}
		return totalWidth;
	}
	else
	{
		return Window::GetHeight();
	}
}

float StackLayout::GetHeight() const
{
	if (FlowDirection::Vertical == _flowDirection)
	{
		float totalHeight = 0;
		auto &children = GetChildren();
		for (auto &item : children)
		{
			totalHeight += item->GetHeight();
		}
		if (children.size() > 1)
		{
			totalHeight += (float)(children.size() - 1) * _spacing;
		}
		return totalHeight;
	}
	else
	{
		return Window::GetHeight();
	}
}

