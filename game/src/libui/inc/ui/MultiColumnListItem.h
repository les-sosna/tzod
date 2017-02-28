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
		explicit MultiColumnListItem(TextureManager &texman);

		void EnsureColumn(TextureManager &texman, unsigned int columnIndex, float offset);

		// Window
		vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale) const override;
		FRECT GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const override;


	private:
		std::shared_ptr<Rectangle> _selection;
		std::vector<std::shared_ptr<Text>> _columns;
	};
}
