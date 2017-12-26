#pragma once
#include "Window.h"

namespace UI
{
	class ScanlineLayout : public Window
	{
	public:
//		ScanlineLayout();

		void SetElementSize(vec2d size) { _elementSize = size; }
		vec2d GetElementSize() const { return _elementSize; }

		// Window
		FRECT GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const override;

	private:
		vec2d _elementSize;
	};
}
