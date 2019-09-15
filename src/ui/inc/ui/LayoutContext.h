#pragma once
#include <math/MyMath.h>
#include <vector>

namespace UI
{
	class Window;
	class DataContext;
	class InputContext;
	struct WindowLayout;

	class LayoutContext
	{
	public:
		// Initialize root layout context
		LayoutContext(float opacity, float scale, vec2d pxOffset, vec2d pxSize, bool enabled, bool focused);

		// Initialize layout context for child window
		LayoutContext(const InputContext& ic, const Window &parentWindow, const LayoutContext &parentLC,
			const Window &childWindow, const WindowLayout &childLayout);

		vec2d GetPixelSize() const { return _pxSize; }
		vec2d GetPixelOffsetCombined() const { return _pxOffsetCombined; }
		bool GetEnabledCombined() const { return _enabledCombined; }
		bool GetFocusedCombined() const { return _focusedCombined; }
		float GetScaleCombined() const { return _scaleCombined; }
		float GetOpacityCombined() const { return _opacityCombined; }

	private:
		vec2d _pxOffsetCombined;
		vec2d _pxSize;
		float _scaleCombined;
		float _opacityCombined;
		bool _enabledCombined;
		bool _focusedCombined;
	};

	inline float ToPx(float units, const LayoutContext& lc)
	{
		return std::floor(units * lc.GetScaleCombined());
	}

	inline float ToPx(float units, float scale)
	{
		return std::floor(units * scale);
	}

	inline vec2d ToPx(vec2d units, const LayoutContext& lc)
	{
		return Vec2dFloor(units * lc.GetScaleCombined());
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
