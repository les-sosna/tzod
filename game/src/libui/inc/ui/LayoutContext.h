#pragma once
#include <math/MyMath.h>
#include <vector>

namespace UI
{
	class Window;

	class LayoutContext
	{
	public:
		LayoutContext(float scale, vec2d size, bool enabled);

		void PushTransform(vec2d offset, vec2d size, bool enabled);
		void PopTransform();

		bool GetEnabled() const { return _layoutStack.back().enabled; }
		vec2d GetPixelOffset() const { return _layoutStack.back().offset; }
		vec2d GetPixelSize() const { return _layoutStack.back().size; }
		float GetScale() const { return _scale; }

	private:
		struct Node
		{
			vec2d offset;
			vec2d size;
			bool enabled;
		};
		std::vector<Node> _layoutStack;
		float _scale;
	};
}
