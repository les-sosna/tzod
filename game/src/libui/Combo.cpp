#include "inc/ui/Combo.h"
#include "inc/ui/Text.h"
#include "inc/ui/List.h"
#include "inc/ui/Button.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/Keys.h"

namespace UI
{

///////////////////////////////////////////////////////////////////////////////

ComboBox::ComboBox(Window *parent, ListDataSource *dataSource)
  : Window(parent)
  , _curSel(-1)
{
	_text = TextButton::Create(this, 0, 1, std::string(), "font_small");
	_text->eventClick = std::bind(&ComboBox::DropList, this);

	_list = List::Create(this, dataSource, 0, 0, 1, 1);
	_list->SetTexture("ui/combo_list", false);
	_list->SetVisible(false);
	_list->SetTopMost(true);
	_list->eventClickItem = std::bind(&ComboBox::OnClickItem, this, std::placeholders::_1);
	_list->eventChangeCurSel = std::bind(&ComboBox::OnChangeSelection, this, std::placeholders::_1);
	_list->eventLostFocus = std::bind(&ComboBox::OnListLostFocus, this);

	_btn = ImageButton::Create(this, 0, 0, "ui/scroll_down");
	_btn->eventClick = std::bind(&ComboBox::DropList, this);

	_text->BringToFront();
	_text->SetDrawShadow(false);

	SetDrawBorder(true);
	SetTexture("ui/combo", false);
	Window::Resize(GetWidth(), _text->GetHeight() + _text->GetY() * 2);
}

ListDataSource* ComboBox::GetData() const
{
	return _list->GetData();
}

void ComboBox::SetCurSel(int index)
{
	_curSel = index;
	_list->SetCurSel(index);
	if( eventChangeCurSel )
		eventChangeCurSel(index);
}

int ComboBox::GetCurSel() const
{
	return _curSel;
}

List* ComboBox::GetList() const
{
	return _list;
}

void ComboBox::DropList()
{
	if( _list->GetVisible() )
	{
		_btn->SetTexture("ui/scroll_down", false);
		_list->SetVisible(false);
		_list->SetCurSel(GetCurSel());
		GetManager().SetFocusWnd(this);
	}
	else
	{
		_btn->SetTexture("ui/scroll_up", false);
		_list->SetVisible(true);
		_list->SetScrollPos((float) GetCurSel());
		GetManager().SetFocusWnd(_list);
	}
}

void ComboBox::OnClickItem(int index)
{
	if( -1 != index )
	{
		_curSel = index;
		_text->SetText(_list->GetData()->GetItemText(index, 0));
		_text->Resize(_btn->GetX(), GetHeight()); // workaround: SetText changes button size
		_list->SetVisible(false);
		_btn->SetTexture("ui/scroll_down", false);
		GetManager().SetFocusWnd(this);

		if( eventChangeCurSel )
			eventChangeCurSel(index);
	}
}

void ComboBox::OnChangeSelection(int index)
{
	if( -1 != index )
	{
		_text->SetText(_list->GetData()->GetItemText(index, 0));
		_text->Resize(_btn->GetX(), GetHeight()); // workaround: SetText changes button size
	}
}

void ComboBox::OnListLostFocus()
{
	_list->SetCurSel(_curSel); // cancel changes
	_list->SetVisible(false);
	_btn->SetTexture("ui/scroll_down", false);
}

void ComboBox::OnEnabledChange(bool enable, bool inherited)
{
	SetFrame(enable ? 0 : 3);
}

bool ComboBox::OnKeyPressed(Key key)
{
	switch( key )
	{
	case Key::Escape:
		if( _list->GetVisible() )
		{
			GetManager().SetFocusWnd(this);
			return true;
		}
		break;
	case Key::Enter:
		if( _list->GetVisible() )
		{
			if( -1 != _list->GetCurSel() )
			{
				OnClickItem(_list->GetCurSel());
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

bool ComboBox::OnFocus(bool focus)
{
	return true;
}

void ComboBox::OnSize(float width, float height)
{
	_list->Move(0, height);
	_list->Resize(width, _list->GetHeight());
	_btn->Move(width - _btn->GetWidth(), (height - _btn->GetHeight()) * 0.5f);
	_text->Resize(_btn->GetX(), height);
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
