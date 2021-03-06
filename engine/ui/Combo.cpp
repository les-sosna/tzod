#include "inc/ui/Combo.h"
#include "inc/ui/DataContext.h"
#include "inc/ui/DataSource.h"
#include "inc/ui/Rectangle.h"
#include "inc/ui/Text.h"
#include "inc/ui/List.h"
#include "inc/ui/ListBox.h"
#include "inc/ui/Button.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/StateContext.h"
#include <plat/Keys.h>
#include <video/TextureManager.h>

using namespace UI;

ComboBox::ComboBox(ListDataSource *dataSource)
  : _curSel(-1)
{
	_background = std::make_shared<Rectangle>();
	_background->SetDrawBorder(true);
	_background->SetTexture("ui/combo");
	AddFront(_background);

	_list = std::make_shared<ListBox>(dataSource);
	_list->SetVisible(false);
	_list->SetTopMost(true);
	_list->GetList()->eventClickItem = std::bind(&ComboBox::OnClickItem, this, std::placeholders::_1);
	AddFront(_list);

	_btn = std::make_shared<Button>();
	_btn->SetBackground("ui/scroll_down");
	_btn->eventClick = std::bind(&ComboBox::DropList, this);
	AddFront(_btn);
}

ListDataSource* ComboBox::GetData() const
{
	return _list->GetList()->GetData();
}

void ComboBox::SetCurSel(int index)
{
	_curSel = index;
	_list->GetList()->SetCurSel(index);
	if( eventChangeCurSel )
		eventChangeCurSel(index);
}

int ComboBox::GetCurSel() const
{
	return _curSel;
}

std::shared_ptr<List> ComboBox::GetList() const
{
	return _list->GetList();
}

void ComboBox::DropList()
{
	if( _list->GetVisible() )
	{
		_btn->SetBackground("ui/scroll_down");
		_list->SetVisible(false);
		_list->GetList()->SetCurSel(GetCurSel());
		SetFocus(nullptr);
	}
	else
	{
		_btn->SetBackground("ui/scroll_up");
		_list->SetVisible(true);
//		_list->SetScrollPos((float) GetCurSel());
		SetFocus(_list.get());
	}
}

void ComboBox::OnClickItem(int index)
{
	if( -1 != index )
	{
		_curSel = index;
		_list->SetVisible(false);
		_btn->SetBackground("ui/scroll_down");
		SetFocus(nullptr);

		if( eventChangeCurSel )
			eventChangeCurSel(index);
	}
}

void ComboBox::OnListLostFocus()
{
	_list->GetList()->SetCurSel(_curSel); // cancel changes
	_list->SetVisible(false);
	_btn->SetBackground("ui/scroll_down");
}

bool ComboBox::OnKeyPressed(const Plat::Input &input, const InputContext &ic, Plat::Key key)
{
	switch( key )
	{
	case Plat::Key::Escape:
		if( _list->GetVisible() )
		{
			SetFocus(nullptr);
			return true;
		}
		break;
	case Plat::Key::Enter:
		if( _list->GetVisible() )
		{
			if( -1 != _list->GetList()->GetCurSel() )
			{
				OnClickItem(_list->GetList()->GetCurSel());
				return true;
			}
		}
		break;
	case Plat::Key::Down:
		if( !_list->GetVisible() )
			DropList();
		return true;
	default:
		break;
	}
	return false;
}

WindowLayout ComboBox::GetChildLayout(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	float scale = lc.GetScaleCombined();
	vec2d size = lc.GetPixelSize();

	if (_list.get() == &child)
	{
		return WindowLayout{ FRECT{ 0, size.y, size.x, size.y + ToPx(_list->GetList()->GetHeight(), scale) }, 1, true };
	}
	else if (_btn.get() == &child)
	{
		vec2d btnSize = _btn->GetBackground().GetTextureSize(texman);
		float top = std::floor((size.y - btnSize.y * scale) / 2);
		return WindowLayout{ MakeRectRB(vec2d{ size.x - ToPx(btnSize.x, scale), top }, vec2d{ size.x, top + ToPx(btnSize.y, scale) }), 1, true };
	}

	assert(false);
	return {};
}

vec2d ComboBox::GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const
{
	DataContext itemDC;
	{
		itemDC.SetItemIndex(_list->GetList()->GetCurSel());
		itemDC.SetDataContext(_list->GetList()->GetData());
	}
	vec2d itemSize = _list->GetList()->GetItemTemplate()->GetContentSize(texman, itemDC, scale, layoutConstraints);
	vec2d pxBtnSize = ToPx(_btn->GetBackground().GetTextureSize(texman), scale);
	return vec2d{ itemSize.x + pxBtnSize.x, std::max(itemSize.y, pxBtnSize.y) };
}

void ComboBox::Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, const Plat::Input &input, float time, bool hovered) const
{
	if (_list->GetList()->GetCurSel() != -1)
	{
		UI::RenderSettings rs{ input, ic, rc, texman, time };

		DataContext itemDC;
		{
			itemDC.SetDataContext(_list->GetList()->GetData());
			itemDC.SetItemIndex(_list->GetList()->GetCurSel());
		}

		vec2d pxItemSize = { lc.GetPixelSize().x - ToPx(_btn->GetWidth(), lc), lc.GetPixelSize().y };
		LayoutContext itemLC(lc.GetOpacityCombined(), lc.GetScaleCombined(), lc.GetPixelOffsetCombined(), pxItemSize, lc.GetEnabledCombined(), lc.GetFocusedCombined());
		RenderUIRoot(_list->GetList()->GetItemTemplate(), rs, itemLC, itemDC, sc);
	}
}

