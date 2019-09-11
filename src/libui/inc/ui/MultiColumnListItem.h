#pragma once
#include "Window.h"
#include <vector>

namespace UI
{
	class Rectangle;
	class Text;

	class MultiColumnListItem : public WindowContainer
	{
	public:
		MultiColumnListItem();

		void EnsureColumn(unsigned int columnIndex, float offset);

		// Window
		vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const override;
		WindowLayout GetChildLayout(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const override;


	private:
		std::shared_ptr<Rectangle> _selection;
		std::vector<std::pair<std::shared_ptr<Text>, float>> _columns;
	};
}
