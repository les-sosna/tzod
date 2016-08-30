#pragma once
#include <math/MyMath.h>
#include <vector>

namespace UI
{
	class Window;

	class LayoutContext
	{
	public:
		LayoutContext(float scale, vec2d offset, vec2d size, bool enabled);
		LayoutContext(const LayoutContext &parent, const Window &parentWindow, const Window &childWindow);

		bool GetEnabled() const { return _enabled; }
		vec2d GetPixelOffset() const { return _offset; }
		vec2d GetPixelSize() const { return _size; }
		float GetScale() const { return _scale; }

	private:
		vec2d _offset;
		vec2d _size;
		float _scale;
		bool _enabled;
	};
}
