#include "inc/ui/LayoutContext.h"
#include "inc/ui/ListBox.h"
#include "inc/ui/List.h"
#include "inc/ui/MultiColumnListItem.h"
#include "inc/ui/Rectangle.h"
#include "inc/ui/ScrollView.h"

using namespace UI;

ListBox::ListBox(LayoutManager &manager, TextureManager &texman, ListDataSource* dataSource)
	: Window(manager)
	, _background(std::make_shared<Rectangle>(manager))
	, _scrollView(std::make_shared<ScrollView>(manager))
	, _list(std::make_shared<List>(manager, texman, dataSource))
{
	AddFront(_background);
	AddFront(_scrollView);

	_list->SetItemTemplate(std::make_shared<UI::MultiColumnListItem>(manager, texman));

	_scrollView->SetContent(_list);
	SetFocus(_scrollView);

	_background->SetTexture(texman, "ui/list", false);
	_background->SetDrawBorder(true);
}

FRECT ListBox::GetChildRect(TextureManager &texman, const LayoutContext &lc, const StateContext &sc, const Window &child) const
{
	float scale = lc.GetScale();
	vec2d size = lc.GetPixelSize();

	if (_background.get() == &child)
	{
		return FRECT{ 0, 0, size.x, size.y };
	}
	else if (_scrollView.get() == &child)
	{
		return FRECT{ 1, 1, size.x - 2, size.y - 2 };
	}

	return Window::GetChildRect(texman, lc, sc, child);
}

