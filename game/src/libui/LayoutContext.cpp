#include "inc/ui/LayoutContext.h"
#include "inc/ui/Window.h"

using namespace UI;

LayoutContext::LayoutContext(float opacity, float scale, vec2d offset, vec2d size, bool enabled)
	: _offset(offset)
	, _size(size)
	, _scale(scale)
	, _opacityCombined(opacity)
	, _enabled(enabled)
{
}

LayoutContext::LayoutContext(const Window &parentWindow, const LayoutContext &parentLC, const Window &childWindow, const FRECT &childRect, const DataContext &childDC)
	: _scale(parentLC.GetScale())
	, _opacityCombined(parentLC.GetOpacityCombined() * parentWindow.GetChildOpacity(childWindow))
	, _enabled(parentLC.GetEnabledCombined() && childWindow.GetEnabled(childDC))
{
	_offset = parentLC.GetPixelOffset() + Offset(childRect);
	_size = Size(childRect);
}
