#pragma once
#include <math/MyMath.h>
#include <vector>

namespace UI
{
	class Window;

	class LayoutContext
	{
	public:
		LayoutContext(vec2d size, bool enabled);

		bool GetEnabled() const { return _enabled; }
		vec2d GetSize() const { return _size; }

	private:
		vec2d _size;
		bool _enabled;
	};
}
