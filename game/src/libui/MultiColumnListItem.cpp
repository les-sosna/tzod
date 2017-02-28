#include "inc/ui/DataSource.h"
#include "inc/ui/ListBase.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/MultiColumnListItem.h"
#include "inc/ui/Rectangle.h"
#include "inc/ui/StateContext.h"
#include "inc/ui/Text.h"

using namespace UI;

MultiColumnListItem::MultiColumnListItem(TextureManager &texman)
	: _selection(std::make_shared<Rectangle>())
{
	static const auto selectionFillColorMap = std::make_shared<UI::StateBinding<SpriteColor>>(0x00000000, // default
		UI::StateBinding<SpriteColor>::MapType{ { "Focused", 0xffffffff } });
	static const auto selectionBorderColorMap = std::make_shared<UI::StateBinding<SpriteColor>>(0x00000000, // default
		UI::StateBinding<SpriteColor>::MapType{ { "Focused", 0xffffffff },{ "Unfocused", 0xffffffff } });

	AddBack(_selection);
	_selection->SetTexture(texman, "ui/listsel", false);
	_selection->SetBackColor(selectionFillColorMap);
	_selection->SetBorderColor(selectionBorderColorMap);

	EnsureColumn(texman, 0u, 0.f);
}

void MultiColumnListItem::EnsureColumn(TextureManager &texman, unsigned int columnIndex, float offset)
{
	static const auto textColorMap = std::make_shared<UI::StateBinding<SpriteColor>>(0xffffffff, // default
		UI::StateBinding<SpriteColor>::MapType{ { "Disabled", 0xbbbbbbbb },{ "Hover", 0xffccccff },{ "Focused", 0xff000000 } });

	if (columnIndex >= _columns.size())
		_columns.insert(_columns.end(), 1 + columnIndex - _columns.size(), nullptr);

	if (!_columns[columnIndex])
	{
		// TODO: reuse the text object
		_columns[columnIndex] = std::make_shared<UI::Text>(texman);
		AddFront(_columns[columnIndex]);
	}

	_columns[columnIndex]->Move(offset, 0);
	_columns[columnIndex]->SetText(std::make_shared<ListDataSourceBinding>(columnIndex));
	_columns[columnIndex]->SetFont(texman, "font_small");
	_columns[columnIndex]->SetFontColor(textColorMap);
}

vec2d MultiColumnListItem::GetContentSize(TextureManager &texman, const DataContext &dc, float scale) const
{
	return _columns[0]->GetContentSize(texman, dc, scale);
}

FRECT MultiColumnListItem::GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	float scale = lc.GetScale();
	vec2d size = lc.GetPixelSize();

	if (_selection.get() == &child)
	{
		vec2d pxMargins = Vec2dFloor(vec2d{ 1, 2 } * scale);
		return MakeRectWH(-pxMargins, size + pxMargins * 2);
	}

	return Window::GetChildRect(texman, lc, dc, child);
}

