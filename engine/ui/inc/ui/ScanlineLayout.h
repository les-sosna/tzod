#pragma once
#include "Navigation.h"
#include "Window.h"

namespace UI
{
	class ScanlineLayout
		: public WindowContainer
		, private NavigationSink
	{
	public:
		void SetElementSize(vec2d size) { _elementSize = size; }
		vec2d GetElementSize() const { return _elementSize; }

		// Window
		NavigationSink* GetNavigationSink() override { return this; }
		WindowLayout GetChildLayout(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const override;
		vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const override;

	private:
		vec2d _elementSize;

		Window* GetNavigateTarget(TextureManager& texman, const LayoutContext &lc, const DataContext& dc, Navigate navigate);

		// NavigationSink
		bool CanNavigate(TextureManager& texman, const LayoutContext& lc, const DataContext& dc, Navigate navigate) const override;
		void OnNavigate(TextureManager& texman, const LayoutContext& lc, const DataContext& dc, Navigate navigate, NavigationPhase phase) override;
	};
}
