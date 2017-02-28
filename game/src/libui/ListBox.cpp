#include "inc/ui/LayoutContext.h"
#include "inc/ui/ListBox.h"
#include "inc/ui/List.h"
#include "inc/ui/MultiColumnListItem.h"
#include "inc/ui/Rectangle.h"
#include "inc/ui/ScrollView.h"

using namespace UI;

static const vec2d c_borderSize = { 1.f, 1.f };

ListBox::ListBox(TextureManager &texman, ListDataSource* dataSource)
	: _background(std::make_shared<Rectangle>())
	, _scrollView(std::make_shared<ScrollView>())
	, _list(std::make_shared<List>(texman, dataSource))
{
	AddFront(_background);
	AddFront(_scrollView);

	_list->SetItemTemplate(std::make_shared<UI::MultiColumnListItem>(texman));

	_scrollView->SetContent(_list);
	SetFocus(_scrollView);

	_background->SetTexture(texman, "ui/list", false);
	_background->SetDrawBorder(true);
}

FRECT ListBox::GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	float scale = lc.GetScale();
	vec2d size = lc.GetPixelSize();

	if (_background.get() == &child)
	{
		return MakeRectWH(size);
	}
	else if (_scrollView.get() == &child)
	{
		vec2d pxBorderSize = Vec2dFloor(c_borderSize * scale);
		return MakeRectRB(pxBorderSize, size - pxBorderSize);
	}

	return Window::GetChildRect(texman, lc, dc, child);
}

vec2d ListBox::GetContentSize(TextureManager &texman, const DataContext &dc, float scale) const
{
	return _scrollView->GetContentSize(texman, dc, scale) + Vec2dFloor(c_borderSize * scale) * 2;
}

