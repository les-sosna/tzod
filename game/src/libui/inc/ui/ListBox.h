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
		ListBox(LayoutManager &manager, TextureManager &texman, ListDataSource* dataSource);

		std::shared_ptr<List> GetList() { return _list; }
		std::shared_ptr<ScrollView> GetScrollView() { return _scrollView; }

		// Window
		FRECT GetChildRect(vec2d size, float scale, const Window &child) const override;

	private:
		std::shared_ptr<Rectangle> _background;
		std::shared_ptr<ScrollView> _scrollView;
		std::shared_ptr<List> _list;
	};
}
