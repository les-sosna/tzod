#pragma once
#include <math/MyMath.h>
#include <vector>

namespace UI
{
	class Window;
	class DataContext;

	class LayoutContext
	{
	public:
		LayoutContext(float opacity, float scale, vec2d size, bool enabled);
		LayoutContext(const Window &parentWindow, const LayoutContext &parentLC, const Window &childWindow, vec2d size, const DataContext &childDC);

		bool GetEnabledCombined() const { return _enabled; }
		vec2d GetPixelSize() const { return _size; }
		float GetScale() const { return _scale; }
		float GetOpacityCombined() const { return _opacityCombined; }

	private:
		vec2d _size;
		float _scale;
		float _opacityCombined;
		bool _enabled;
	};

	inline float ToPx(float units, const LayoutContext& lc)
	{
		return std::floor(units * lc.GetScale());
	}

	inline float ToPx(float units, float scale)
	{
		return std::floor(units * scale);
	}

	inline vec2d ToPx(vec2d units, const LayoutContext& lc)
	{
		return Vec2dFloor(units * lc.GetScale());
	}

	inline vec2d ToPx(vec2d units, float scale)
	{
		return Vec2dFloor(units * scale);
	}

	struct LayoutConstraints
	{
		vec2d maxPixelSize;
	};

	inline LayoutConstraints DefaultLayoutConstraints(const LayoutContext &lc)
	{
		LayoutConstraints constraints;
		constraints.maxPixelSize = lc.GetPixelSize();
		return constraints;
	}
}
