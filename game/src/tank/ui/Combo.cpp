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
  : Window(parent)
{
	_text = new Text(this, 0, 1, "Combo", alignTextLT);

	SetBorder(true);
	Move(x, y);
	Resize(width, _text->GetHeight()+2);

	_list = new List(this, 0, GetHeight()+2, width, _text->GetHeight()*6);
	_list->Show(false);
	_list->SetTopMost(true);
	_list->eventClickItem.bind(&ComboBox::OnSelectItem, this);

	_btn = new ImageButton(this, 0, 0, "ctrl_scroll_down");
	_btn->Move(GetWidth() - _btn->GetWidth(), (GetHeight() - _btn->GetHeight()) * 0.5f);
	_btn->eventClick.bind(&ComboBox::DropList, this);
}

List* ComboBox::GetList() const
{
	return _list;
}

void  ComboBox::DropList()
{
	if( _list->IsVisible() )
	{
		_list->Show(false);
		GetManager()->SetFocusWnd(this);
	}
	else
	{
		_list->Show(true);
		GetManager()->SetFocusWnd(_list);
	}
}

void  ComboBox::OnSelectItem(int index)
{
	_text->SetText( _list->GetItemText(index, 0).c_str() );
	_list->Show(false);

	if( eventChangeCurSel )
		INVOKE(eventChangeCurSel) ();
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
