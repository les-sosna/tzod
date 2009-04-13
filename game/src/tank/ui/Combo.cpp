// Combo.cpp

#include "stdafx.h"

#include "Combo.h"
#include "Text.h"
#include "List.h"
#include "Button.h"

#include "GuiManager.h"


namespace UI
{

///////////////////////////////////////////////////////////////////////////////

ComboBox::ComboBox(Window *parent, float x, float y, float width)
  : Window(parent, x, y, "ctrl_list")
  , _curSel(-1)
{
	_text = new TextButton(this, 0, 1, string_t(), "font_small");
	_text->eventClick.bind(&ComboBox::DropList, this);

	_list = new List(this, 0, 0, 1, 1);
	_list->SetVisible(false);
	_list->SetTopMost(true);
	_list->eventClickItem.bind(&ComboBox::OnClickItem, this);
	_list->eventChangeCurSel.bind(&ComboBox::OnChangeSelection, this);
	_list->eventLostFocus.bind(&ComboBox::OnListLostFocus, this);

	_btn = new ImageButton(this, 0, 0, "ctrl_scroll_down");
	_btn->eventClick.bind(&ComboBox::DropList, this);

	_text->BringToFront();

	SetBorder(true);
	Resize(width, _text->GetHeight()+2);
}

void ComboBox::SetCurSel(int index)
{
	_curSel = index;
	_list->SetCurSel(index);
	if( eventChangeCurSel )
		INVOKE(eventChangeCurSel) (index);
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
		_list->SetVisible(false);
		_list->SetCurSel(GetCurSel());
		GetManager()->SetFocusWnd(this);
	}
	else
	{
		_list->SetVisible(true);
		_list->SetScrollPos((float) GetCurSel());
		GetManager()->SetFocusWnd(_list);
	}
}

void ComboBox::OnClickItem(int index)
{
	if( -1 != index )
	{
		_curSel = index;
		_text->SetText(_list->GetItemText(index, 0));
		_text->Resize(_btn->GetX(), GetHeight()); // workaround: SetText changes button size
		_list->SetVisible(false);
		GetManager()->SetFocusWnd(this);

		if( eventChangeCurSel )
			INVOKE(eventChangeCurSel) (index);
	}
}

void ComboBox::OnChangeSelection(int index)
{
	if( -1 != index )
	{
		_text->SetText(_list->GetItemText(index, 0));
		_text->Resize(_btn->GetX(), GetHeight()); // workaround: SetText changes button size
	}
}

void ComboBox::OnListLostFocus()
{
	_list->SetCurSel(_curSel); // cancel changes
	_list->SetVisible(false);
}

void ComboBox::OnRawChar(int c)
{
	switch(c)
	{
	case VK_ESCAPE:
		if( _list->GetVisible() )
			GetManager()->SetFocusWnd(this);
		else
			GetParent()->OnRawChar(c);
		break;
	case VK_RETURN:
		if( _list->GetVisible() )
		{
			if( -1 != _list->GetCurSel() )
			{
				OnClickItem(_list->GetCurSel());
			}
		}
		else
		{
			GetParent()->OnRawChar(c);
		}
		break;
	case VK_DOWN:
		if( !_list->GetVisible() )
			DropList();
		break;
	default:
		GetParent()->OnRawChar(c);
	}
}

bool ComboBox::OnFocus(bool focus)
{
	return true;
}

void ComboBox::OnSize(float width, float height)
{
	_list->Move(0, height + 2);
	_list->Resize(width, _list->GetHeight());
	_btn->Move(width - _btn->GetWidth(), (height - _btn->GetHeight()) * 0.5f);
	_text->Resize(_btn->GetX(), height);
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
