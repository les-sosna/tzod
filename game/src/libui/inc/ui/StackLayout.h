#pragma once
#include "Window.h"

namespace UI
{
	enum class Align
	{
		LT, CT, RT,
		LC, CC, RC,
		LB, CB, RB,
	};

	class StackLayout : public Window
	{
	public:
		explicit StackLayout(LayoutManager &manager);

		void SetSpacing(float spacing) { _spacing = spacing; }
		float GetSpacing() const { return _spacing; }

		void SetFlowDirection(FlowDirection flowDirection) { _flowDirection = flowDirection; }
		FlowDirection GetFlowDirection() const { return _flowDirection; }

		void SetAlign(Align align) { _align = align; }
		Align GetAlign() const { return _align; }

		// Window
		FRECT GetChildRect(TextureManager &texman, const LayoutContext &lc, const StateContext &sc, const Window &child) const override;
		vec2d GetContentSize(TextureManager &texman, const StateContext &sc, float scale) const override;

	private:
		float _spacing = 0.f;
		FlowDirection _flowDirection = FlowDirection::Vertical;
		Align _align = Align::LT;
	};
}
