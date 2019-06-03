#include "inc/ui/LayoutContext.h"
#include "inc/ui/ListBox.h"
#include "inc/ui/List.h"
#include "inc/ui/MultiColumnListItem.h"
#include "inc/ui/Rectangle.h"
#include "inc/ui/ScrollView.h"

using namespace UI;

static const vec2d c_borderSize = { 1.f, 1.f };

ListBox::ListBox(ListDataSource* dataSource)
	: _background(std::make_shared<Rectangle>())
	, _scrollView(std::make_shared<ScrollView>())
	, _list(std::make_shared<List>(dataSource))
{
	AddFront(_background);
	AddFront(_scrollView);

	_list->SetItemTemplate(std::make_shared<UI::MultiColumnListItem>());

	_scrollView->SetContent(_list);

	_background->SetTexture("ui/list");
	_background->SetDrawBorder(true);
}

WindowLayout ListBox::GetChildLayout(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	float scale = lc.GetScaleCombined();
	vec2d size = lc.GetPixelSize();

	if (_background.get() == &child)
	{
		return WindowLayout{ MakeRectWH(size), 1, true };
	}
	else if (_scrollView.get() == &child)
	{
		vec2d pxBorderSize = Vec2dFloor(c_borderSize * scale);
		return WindowLayout{ MakeRectRB(pxBorderSize, size - pxBorderSize), 1, true };
	}

	assert(false);
	return {};
}

vec2d ListBox::GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const
{
	return _scrollView->GetContentSize(texman, dc, scale, layoutConstraints) + Vec2dFloor(c_borderSize * scale) * 2;
}

std::shared_ptr<const Window> ListBox::GetFocus(const std::shared_ptr<const Window>& owner) const
{
	return _scrollView;
}

const Window* ListBox::GetFocus() const
{
	return _scrollView.get();
}
