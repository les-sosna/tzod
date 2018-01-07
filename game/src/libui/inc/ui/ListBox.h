#pragma once
#include "Window.h"

namespace UI
{
	class List;
	class Rectangle;
	class ScrollView;
	struct ListDataSource;

	class ListBox : public Window
	{
	public:
		explicit ListBox(ListDataSource* dataSource);

		std::shared_ptr<List> GetList() { return _list; }
		std::shared_ptr<ScrollView> GetScrollView() { return _scrollView; }

		// Window
		FRECT GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const override;
		vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const override;
		std::shared_ptr<Window> GetFocus() const override;

	private:
		std::shared_ptr<Rectangle> _background;
		std::shared_ptr<ScrollView> _scrollView;
		std::shared_ptr<List> _list;
	};
}
