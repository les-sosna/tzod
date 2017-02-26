#include "inc/ui/LayoutContext.h"
#include "inc/ui/Window.h"

using namespace UI;

LayoutContext::LayoutContext(float opacity, float scale, vec2d size, bool enabled)
	: _size(size)
	, _scale(scale)
	, _opacityCombined(opacity)
	, _enabled(enabled)
{
}

LayoutContext::LayoutContext(const Window &parentWindow, const LayoutContext &parentLC, const Window &childWindow, vec2d size, const DataContext &childDC)
	: _size(size)
	, _scale(parentLC.GetScale())
	, _opacityCombined(parentLC.GetOpacityCombined() * parentWindow.GetChildOpacity(childWindow))
	, _enabled(parentLC.GetEnabledCombined() && childWindow.GetEnabled(childDC))
{
}
