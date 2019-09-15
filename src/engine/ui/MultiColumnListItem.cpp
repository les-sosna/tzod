#include "inc/ui/DataSource.h"
#include "inc/ui/ListBase.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/MultiColumnListItem.h"
#include "inc/ui/Rectangle.h"
#include "inc/ui/StateContext.h"
#include "inc/ui/Text.h"

using namespace UI;

MultiColumnListItem::MultiColumnListItem()
	: _selection(std::make_shared<Rectangle>())
{
	static const auto selectionFillColorMap = std::make_shared<UI::StateBinding<SpriteColor>>(0x00000000, // default
		UI::StateBinding<SpriteColor>::MapType{ { "Focused", 0xffffffff } });
	static const auto selectionBorderColorMap = std::make_shared<UI::StateBinding<SpriteColor>>(0x00000000, // default
		UI::StateBinding<SpriteColor>::MapType{ { "Focused", 0xffffffff },{ "Unfocused", 0xffffffff } });

	AddBack(_selection);
	_selection->SetTexture("ui/listsel");
	_selection->SetBackColor(selectionFillColorMap);
	_selection->SetBorderColor(selectionBorderColorMap);

	EnsureColumn(0u, 0.f);
}

void MultiColumnListItem::EnsureColumn(unsigned int columnIndex, float offset)
{
	static const auto textColorMap = std::make_shared<UI::StateBinding<SpriteColor>>(0xffffffff, // default
		UI::StateBinding<SpriteColor>::MapType{ { "Disabled", 0xbbbbbbbb },{ "Hover", 0xffccccff },{ "Focused", 0xff000000 } });

	if (columnIndex >= _columns.size())
		_columns.insert(_columns.end(), 1 + columnIndex - _columns.size(), {});

	if (!_columns[columnIndex].first)
	{
		// TODO: reuse the text object
		_columns[columnIndex].first = std::make_shared<UI::Text>();
		AddFront(_columns[columnIndex].first);
	}

	_columns[columnIndex].first->SetText(std::make_shared<ListDataSourceBinding>(columnIndex));
	_columns[columnIndex].first->SetFont("font_small");
	_columns[columnIndex].first->SetFontColor(textColorMap);
	_columns[columnIndex].second = offset;
}

vec2d MultiColumnListItem::GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const
{
	return _columns[0].first->GetContentSize(texman, dc, scale, layoutConstraints);
}

WindowLayout MultiColumnListItem::GetChildLayout(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	float scale = lc.GetScaleCombined();
	vec2d size = lc.GetPixelSize();

	if (_selection.get() == &child)
	{
		vec2d pxMargins = Vec2dFloor(vec2d{ 1, 2 } * scale);
		return WindowLayout{ MakeRectWH(-pxMargins, size + pxMargins * 2), 1, true };
	}

	for (size_t i = 0; i != _columns.size(); ++i)
	{
		if (_columns[i].first.get() == &child)
		{
			float nextColumnOffset = (i + 1 != _columns.size()) ? ToPx(_columns[i + 1].second, lc) : lc.GetPixelSize().x;
			return WindowLayout{ MakeRectRB(vec2d{ToPx(_columns[i].second, lc), 0}, vec2d{ nextColumnOffset, lc.GetPixelSize().y }), 1, true };
		}
	}

	assert(false);
	return {};
}

