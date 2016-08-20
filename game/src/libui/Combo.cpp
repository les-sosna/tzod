#include "inc/ui/Combo.h"
#include "inc/ui/DataSource.h"
#include "inc/ui/Text.h"
#include "inc/ui/List.h"
#include "inc/ui/ListBox.h"
#include "inc/ui/Button.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/Keys.h"
#include "inc/ui/LayoutContext.h"
#include <video/TextureManager.h>

using namespace UI;

ComboBox::ComboBox(LayoutManager &manager, TextureManager &texman, ListDataSource *dataSource)
  : Rectangle(manager)
  , _curSel(-1)
{
	_list = std::make_shared<ListBox>(manager, texman, dataSource);
	_list->SetVisible(false);
	_list->SetTopMost(true);
	_list->GetList()->eventClickItem = std::bind(&ComboBox::OnClickItem, this, std::placeholders::_1);
	_list->GetList()->eventChangeCurSel = std::bind(&ComboBox::OnChangeSelection, this, std::placeholders::_1);
	_list->GetList()->eventLostFocus = std::bind(&ComboBox::OnListLostFocus, this);
	AddFront(_list);

	_btn = std::make_shared<Button>(manager, texman);
	_btn->SetBackground(texman, "ui/scroll_down", true);
	_btn->eventClick = std::bind(&ComboBox::DropList, this);
	AddFront(_btn);

	auto fontName = "font_small";

	_text = std::make_shared<TextButton>(manager, texman);
	_text->SetFont(texman, fontName);
	_text->eventClick = std::bind(&ComboBox::DropList, this);
	AddFront(_text);

	SetDrawBorder(true);
	SetTexture(texman, "ui/combo", false);
	Rectangle::Resize(GetWidth(), texman.GetSpriteInfo(texman.FindSprite(fontName)).pxFrameHeight + 2);
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
		_btn->SetBackground(GetManager().GetTextureManager(), "ui/scroll_down", false);
		_list->SetVisible(false);
		_list->GetList()->SetCurSel(GetCurSel());
		SetFocus(nullptr);
	}
	else
	{
		_btn->SetBackground(GetManager().GetTextureManager(), "ui/scroll_up", false);
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
		_text->SetText(std::make_shared<StaticText>(_list->GetList()->GetData()->GetItemText(index, 0)));
		_list->SetVisible(false);
		_btn->SetBackground(GetManager().GetTextureManager(), "ui/scroll_down", false);
		SetFocus(nullptr);

		if( eventChangeCurSel )
			eventChangeCurSel(index);
	}
}

void ComboBox::OnChangeSelection(int index)
{
	if( -1 != index )
	{
		_text->SetText(std::make_shared<StaticText>(_list->GetList()->GetData()->GetItemText(index, 0)));
	}
}

void ComboBox::OnListLostFocus()
{
	_list->GetList()->SetCurSel(_curSel); // cancel changes
	_list->SetVisible(false);
	_btn->SetBackground(GetManager().GetTextureManager(), "ui/scroll_down", false);
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

FRECT ComboBox::GetChildRect(vec2d size, float scale, const Window &child) const
{
	if (_list.get() == &child)
	{
		return FRECT{ 0, size.y, size.x, size.y + std::floor(_list->GetList()->GetHeight() * scale) };
	}
	else if (_btn.get() == &child)
	{
		float top = std::floor((size.y - child.GetHeight() * scale) / 2);
		return FRECT{ size.x - std::floor(child.GetWidth() * scale), top, size.x, top + std::floor(child.GetHeight() * scale) };
	}
	else if (_text.get() == &child)
	{
		return FRECT{ 0, 0, size.x - std::floor(_btn->GetWidth() * scale), size.y };
	}

	return Window::GetChildRect(size, scale, child);
}

void ComboBox::Draw(const StateContext &sc, const LayoutContext &lc, const InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	const_cast<ComboBox*>(this)->SetFrame(lc.GetEnabled() ? 0 : 3);

	Rectangle::Draw(sc, lc, ic, dc, texman);
}

