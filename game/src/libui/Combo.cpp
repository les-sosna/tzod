#include "inc/ui/Combo.h"
#include "inc/ui/DataContext.h"
#include "inc/ui/DataSource.h"
#include "inc/ui/Text.h"
#include "inc/ui/List.h"
#include "inc/ui/ListBox.h"
#include "inc/ui/Button.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/Keys.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/StateContext.h"
#include <video/TextureManager.h>

using namespace UI;

ComboBox::ComboBox(LayoutManager &manager, TextureManager &texman, ListDataSource *dataSource)
  : Managerful(manager)
  , _curSel(-1)
{
	_list = std::make_shared<ListBox>(texman, dataSource);
	_list->SetVisible(false);
	_list->SetTopMost(true);
	_list->GetList()->eventClickItem = std::bind(&ComboBox::OnClickItem, this, std::placeholders::_1);
	_list->GetList()->eventLostFocus = std::bind(&ComboBox::OnListLostFocus, this);
	AddFront(_list);

	_btn = std::make_shared<Button>(texman);
	_btn->SetBackground("ui/scroll_down");
	_btn->AlignToBackground(texman);
	_btn->eventClick = std::bind(&ComboBox::DropList, this);
	AddFront(_btn);

	SetDrawBorder(true);
	SetTexture("ui/combo");
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
		SetFocus(_list);
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

bool ComboBox::OnKeyPressed(InputContext &ic, Key key)
{
	switch( key )
	{
	case Key::Escape:
		if( _list->GetVisible() )
		{
			SetFocus(nullptr);
			return true;
		}
		break;
	case Key::Enter:
		if( _list->GetVisible() )
		{
			if( -1 != _list->GetList()->GetCurSel() )
			{
				OnClickItem(_list->GetList()->GetCurSel());
				return true;
			}
		}
		break;
	case Key::Down:
		if( !_list->GetVisible() )
			DropList();
		return true;
	default:
		break;
	}
	return false;
}

FRECT ComboBox::GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const
{
	float scale = lc.GetScale();
	vec2d size = lc.GetPixelSize();

	if (_list.get() == &child)
	{
		return FRECT{ 0, size.y, size.x, size.y + std::floor(_list->GetList()->GetHeight() * scale) };
	}
	else if (_btn.get() == &child)
	{
		float top = std::floor((size.y - _btn->GetHeight() * scale) / 2);
		return FRECT{ size.x - std::floor(_btn->GetWidth() * scale), top, size.x, top + std::floor(_btn->GetHeight() * scale) };
	}

	return Window::GetChildRect(texman, lc, dc, child);
}

vec2d ComboBox::GetContentSize(TextureManager &texman, const DataContext &dc, float scale) const
{
	DataContext itemDC;
	{
		int sel = _list->GetList()->GetCurSel();
		itemDC.SetItemIndex(sel != -1 ? sel : 0);
		itemDC.SetDataContext(_list->GetList()->GetData());
	}
	vec2d itemSize = _list->GetList()->GetItemTemplate()->GetContentSize(texman, itemDC, scale);
	vec2d btnSize = ToPx(_btn->GetSize(), scale);
	return vec2d{ itemSize.x + btnSize.x, std::max(itemSize.y, btnSize.y) };
}

void ComboBox::Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman) const
{
	Rectangle::Draw(dc, sc, lc, ic, rc, texman);

	if (_list->GetList()->GetCurSel() != -1)
	{
		// TODO: something smarter than const_cast (fork?)
		UI::RenderSettings rs{ const_cast<InputContext&>(ic), rc, texman };

		DataContext itemDC;
		{
			itemDC.SetDataContext(_list->GetList()->GetData());
			itemDC.SetItemIndex(_list->GetList()->GetCurSel());
		}

		vec2d pxItemSize = { lc.GetPixelSize().x - ToPx(_btn->GetWidth(), lc), lc.GetPixelSize().y };
		LayoutContext itemLC(lc.GetOpacityCombined(), lc.GetScale(), pxItemSize, lc.GetEnabledCombined());
		RenderUIRoot(*_list->GetList()->GetItemTemplate(), rs, itemLC, itemDC, sc);
	}
}

