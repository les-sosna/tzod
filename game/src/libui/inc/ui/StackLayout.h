#pragma once
#include "Navigation.h"
#include "Window.h"

namespace UI
{
	enum class Align
	{
		LT, CT, RT,
		LC, CC, RC,
		LB, CB, RB,
	};

	class StackLayout
		: public Window
		, private NavigationSink
	{
	public:
		void SetSpacing(float spacing) { _spacing = spacing; }
		float GetSpacing() const { return _spacing; }

		void SetFlowDirection(FlowDirection flowDirection) { _flowDirection = flowDirection; }
		FlowDirection GetFlowDirection() const { return _flowDirection; }

		void SetAlign(Align align) { _align = align; }
		Align GetAlign() const { return _align; }

		// Window
		NavigationSink* GetNavigationSink() override { return this; }
		FRECT GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const override;
		vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale) const override;

	private:
		float _spacing = 0.f;
		FlowDirection _flowDirection = FlowDirection::Vertical;
		Align _align = Align::LT;

		std::shared_ptr<Window> GetNavigateTarget(const DataContext &dc, Navigate navigate) const;

		// NavigationSink
		bool CanNavigate(Navigate navigate, const DataContext &dc) const override;
		void OnNavigate(Navigate navigate, NavigationPhase phase, const DataContext &dc) override;
	};
}
