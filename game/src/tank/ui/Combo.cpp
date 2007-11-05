// Combo.cpp

#include "stdafx.h"

#include "Combo.h"
#include "Text.h"
#include "List.h"
#include "Button.h"

#include "GuiManager.h"


namespace UI
{
;

///////////////////////////////////////////////////////////////////////////////

ComboBox::ComboBox(Window *parent, float x, float y, float width)
  : Window(parent, x, y, "ctrl_list")
{
	_curSel = -1;

	_text = new Text(this, 0, 1, "", alignTextLT);

	SetBorder(true);
	Resize(width, _text->GetHeight()+2);

	_list = new List(this, 0, GetHeight()+2, width, _text->GetHeight()*6);
	_list->Show(false);
	_list->SetTopMost(true);
	_list->eventClickItem.bind(&ComboBox::OnClickItem, this);
	_list->eventChangeCurSel.bind(&ComboBox::OnChangeSelection, this);
	_list->eventLostFocus.bind(&ComboBox::OnListLostFocus, this);

	_btn = new ImageButton(this, 0, 0, "ctrl_scroll_down");
	_btn->Move(GetWidth() - _btn->GetWidth(), (GetHeight() - _btn->GetHeight()) * 0.5f);
	_btn->eventClick.bind(&ComboBox::DropList, this);

	_btn1 = new ImageButton(this, 0, 0, NULL);
	_btn1->Resize(GetWidth(), GetHeight());
	_btn1->eventClick.bind(&ComboBox::DropList, this);

	_text->BringToFront();
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
	if( _list->IsVisible() )
	{
		_list->Show(false);
		_list->SetCurSel(_curSel);
		GetManager()->SetFocusWnd(this);
	}
	else
	{
		_list->Show(true);
		GetManager()->SetFocusWnd(_list);
	}
}

void ComboBox::OnClickItem(int index)
{
	if( -1 != index )
	{
		_curSel = index;
		_text->SetText( _list->GetItemText(index, 0).c_str() );
		_list->Show(false);
		GetManager()->SetFocusWnd(this);

		if( eventChangeCurSel )
			INVOKE(eventChangeCurSel) (index);
	}
}

void ComboBox::OnChangeSelection(int index)
{
	if( -1 != index )
	{
		_text->SetText( _list->GetItemText(index, 0).c_str() );
	}
}

void ComboBox::OnListLostFocus()
{
	_list->SetCurSel(_curSel); // cancel changes
	_list->Show(false);
}

void ComboBox::OnRawChar(int c)
{
	switch(c)
	{
	case VK_ESCAPE:
		if( _list->IsVisible() )
			GetManager()->SetFocusWnd(this);
		else
			GetParent()->OnRawChar(c);
		break;
	case VK_RETURN:
		if( _list->IsVisible() )
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
		if( !_list->IsVisible() )
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

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
