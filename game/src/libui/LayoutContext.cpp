#include "inc/ui/LayoutContext.h"

using namespace UI;

LayoutContext::LayoutContext(float scale, vec2d size, bool enabled)
	: _layoutStack({ Node{vec2d{}, size, enabled} })
	, _scale(scale)
{}

void LayoutContext::PushTransform(vec2d offset, vec2d size, bool enabled)
{
	assert(!_layoutStack.empty());
	_layoutStack.push_back(Node{
		GetPixelOffset() + offset,
		size,
		GetEnabled() && enabled
	});
}

void LayoutContext::PopTransform()
{
	_layoutStack.pop_back();
	assert(!_layoutStack.empty());
}
