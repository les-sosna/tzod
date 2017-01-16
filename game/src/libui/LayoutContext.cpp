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

LayoutContext::LayoutContext(TextureManager &texman, const Window &parentWindow, const LayoutContext &parentLC, const StateContext &parentSC, const Window &childWindow)
	: _scale(parentLC.GetScale())
	, _enabled(parentLC.GetEnabledCombined() && childWindow.GetEnabled())
{
	auto childRect = parentWindow.GetChildRect(texman, parentLC, parentSC, childWindow);
	_offset = parentLC.GetPixelOffset() + Offset(childRect);
	_size = Size(childRect);
}
