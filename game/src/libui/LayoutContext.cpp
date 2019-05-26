#include "inc/ui/LayoutContext.h"
#include "inc/ui/Window.h"

using namespace UI;

LayoutContext::LayoutContext(float opacity, float scale, vec2d pxOffset, vec2d pxSize, bool enabled, bool focused)
	: _pxOffsetCombined(pxOffset)
	, _pxSize(pxSize)
	, _scaleCombined(scale)
	, _opacityCombined(opacity)
	, _enabledCombined(enabled)
	, _focusedCombined(focused)
{
}

LayoutContext::LayoutContext(const InputContext& ic, const Window &parentWindow, const LayoutContext &parentLC, const Window &childWindow, const DataContext& childDC, vec2d pxChildOffset, vec2d pxChildSize)
	: _pxOffsetCombined(parentLC.GetPixelOffsetCombined() + pxChildOffset)
	, _pxSize(pxChildSize)
	, _scaleCombined(parentLC.GetScaleCombined())
	, _opacityCombined(parentLC.GetOpacityCombined() * parentWindow.GetChildOpacity(parentLC, ic, childWindow))
	, _enabledCombined(parentLC.GetEnabledCombined() && childWindow.GetEnabled(childDC))
	, _focusedCombined(parentLC.GetFocusedCombined() && (parentWindow.GetFocus().get() == &childWindow))
{
}
