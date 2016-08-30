#include "inc/ui/LayoutContext.h"
#include "inc/ui/Window.h"

using namespace UI;

LayoutContext::LayoutContext(float scale, vec2d offset, vec2d size, bool enabled)
	: _offset(offset)
	, _size(size)
	, _scale(scale)
	, _enabled(enabled)
{
}

LayoutContext::LayoutContext(const LayoutContext &parentContext, const Window &parentWindow, const Window &childWindow)
	: _scale(parentContext.GetScale())
	, _enabled(parentContext.GetEnabled() && childWindow.GetEnabled())
{
	auto childRect = parentWindow.GetChildRect(parentContext, childWindow);
	_offset = parentContext.GetPixelOffset() + Offset(childRect);
	_size = Size(childRect);
}
