#pragma once
#include "Navigation.h"
#include "Window.h"

namespace UI
{
	class ScanlineLayout
		: public Window
		, private NavigationSink
	{
	public:
		void SetElementSize(vec2d size) { _elementSize = size; }
		vec2d GetElementSize() const { return _elementSize; }

		// Window
		bool HasNavigationSink() const override { return true; }
		NavigationSink* GetNavigationSink() override { return this; }
		FRECT GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const override;
		vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const override;

	private:
		vec2d _elementSize;

		std::shared_ptr<Window> GetNavigateTarget(const LayoutContext &lc, Navigate navigate);

		// NavigationSink
		bool CanNavigate(Navigate navigate, const LayoutContext &lc, const DataContext &dc) const override;
		void OnNavigate(Navigate navigate, NavigationPhase phase, const LayoutContext &lc, const DataContext &dc) override;
	};
}
