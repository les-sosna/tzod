#pragma once
#include "Window.h"

namespace UI
{
	enum class FlowDirection
	{
		Vertical,
		Horizontal
	};

	class StackLayout : public Window
	{
	public:
		explicit StackLayout(LayoutManager &manager);

		void SetSpacing(float spacing) { _spacing = spacing; }
		float GetSpacing() const { return _spacing; }

		void SetFlowDirection(FlowDirection flowDirection) { _flowDirection = flowDirection; }
		FlowDirection GetFlowDirection() const { return _flowDirection; }

		// Window
		FRECT GetChildRect(vec2d size, float scale, const Window &child) const override;
		float GetWidth() const override;
		float GetHeight() const override;

	private:
		float _spacing = 0.f;
		FlowDirection _flowDirection = FlowDirection::Vertical;
	};
}
