#pragma once
#include <math/MyMath.h>
#include <vector>

namespace UI
{
	class Window;

	class LayoutContext
	{
	public:
		LayoutContext(vec2d size, bool enabled, bool focused, bool hovered);

		bool GetEnabled() const { return _enabled; }
		bool GetHovered() const { return _hovered; }
		bool GetFocused() const { return _focused; }
		vec2d GetSize() const { return _size; }

	private:
		vec2d _size;
		bool _enabled;
		bool _focused;
		bool _hovered;
	};
}
