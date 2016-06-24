#pragma once
#include <math/MyMath.h>
#include <vector>

namespace UI
{
	class Window;

	class LayoutContext
	{
	public:
		LayoutContext(vec2d size, bool enabled, bool hovered);

		bool GetEnabled() const { return _enabled; }
		bool GetHovered() const { return _hovered; }
		vec2d GetSize() const { return _size; }

	private:
		vec2d _size;
		bool _enabled;
		bool _hovered;
	};
}
