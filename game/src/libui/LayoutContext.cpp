#include "inc/ui/LayoutContext.h"

using namespace UI;

LayoutContext::LayoutContext(vec2d size, bool enabled, bool focused, bool hovered)
	: _size(size)
	, _enabled(enabled)
	, _focused(focused)
	, _hovered(hovered)
{}

