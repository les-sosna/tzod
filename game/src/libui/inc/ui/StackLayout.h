#pragma once
#include "Window.h"

namespace UI
{
	class StackLayout : public Window
	{
	public:
		explicit StackLayout(LayoutManager &manager);

		void SetSpacing(float spacing) { _spacing = spacing; }
		float GetSpacing() const { return _spacing; }

		void SetFlowDirection(FlowDirection flowDirection) { _flowDirection = flowDirection; }
		FlowDirection GetFlowDirection() const { return _flowDirection; }

		// Window
		FRECT GetChildRect(const LayoutContext &lc, const Window &child) const override;
		float GetWidth() const override;
		float GetHeight() const override;

	private:
		float _spacing = 0.f;
		FlowDirection _flowDirection = FlowDirection::Vertical;
	};
}
