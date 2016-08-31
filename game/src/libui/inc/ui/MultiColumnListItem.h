#pragma once
#include "Window.h"
#include <vector>

namespace UI
{
	class Rectangle;
	class Text;

	class MultiColumnListItem : public Window
	{
	public:
		MultiColumnListItem(LayoutManager &manager, TextureManager &texman);

		void EnsureColumn(LayoutManager &manager, TextureManager &texman, unsigned int columnIndex, float offset);

		// Window
		vec2d GetContentSize(TextureManager &texman, const StateContext &sc, float scale) const override;
		FRECT GetChildRect(TextureManager &texman, const LayoutContext &lc, const StateContext &sc, const Window &child) const override;


	private:
		std::shared_ptr<Rectangle> _selection;
		std::vector<std::shared_ptr<Text>> _columns;
	};
}
