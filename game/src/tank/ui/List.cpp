// List.cpp

#include "stdafx.h"

#include "List.h"
#include "Scroll.h"
#include "Text.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

List::List(Window *parent, float x, float y, float width, float height)
  : Window(parent)
{
	ClipChildren(true);
	SetBorder(true);
	SetTabPos(0, 1); // first column

	_selection = new Window(this, 0, 0, "ctrl_listsel");

	_scrollBar = new ScrollBar(this, 0, 0, height);
	_scrollBar->Move(width - _scrollBar->GetWidth(), 0);
	_scrollBar->eventScroll.bind(&List::OnScroll, this);

	_blankText = new Text(this, 0, 0, " ", alignTextLT);
	_blankText->Show(false);

	Resize(width, height); // it will resize _selection also, so create it first!
	Move(x, y);

	_curSel = -1;
	UpdateSelection();
}

void List::OnScroll(float pos)
{
	UpdateSelection();
}

void List::UpdateSelection()
{
	float y = (float) _curSel - _scrollBar->GetPos();
	_selection->Move(0, y * _blankText->GetHeight() - 1);
	_selection->Show( -1 != _curSel && y > -1
		&& y < GetHeight() / _blankText->GetHeight() );
}

void List::DeleteAllItems()
{
	_items.clear();
	_scrollBar->SetLimit( (float) _items.size() - GetHeight() / _blankText->GetHeight() );
	SetCurSel(-1, false);
}

int  List::AddItem(const char *str, UINT_PTR data)
{
	Item i;
	i.text.push_back(str);
	i.data = data;
	_items.push_back(i);
	_scrollBar->SetLimit( (float) _items.size() - GetHeight() / _blankText->GetHeight() );

	return _items.size() - 1;
}

void List::SetItemText(int index, int sub, const char *str)
{
	_ASSERT(index >= 0 && index < (int) _items.size());
	if( sub >= (int) _items[index].text.size() )
		_items[index].text.insert(_items[index].text.end(), 1+sub - _items[index].text.size(), "");
	_items[index].text[sub] = str;
}

string_t List::GetItemText(int index, int sub) const
{
	_ASSERT(index >= 0 && index < (int) _items.size());
	_ASSERT(sub >= 0 && sub < (int) _items[index].text.size());
	return _items[index].text[sub];
}

void List::SetTabPos(int index, float pos)
{
	_ASSERT(index >= 0);
	if( index >= (int) _tabs.size() )
		_tabs.insert(_tabs.end(), 1+index - _tabs.size(), pos);
	else
		_tabs[index] = pos;
}

void List::SetItemData(int index, ULONG_PTR data)
{
	_ASSERT(index >= 0);
	_items[index].data = data;
}

ULONG_PTR List::GetItemData(int index)
{
	_ASSERT(index >= 0);
	return _items[index].data;
}

int  List::GetSize() const
{
	return (int) _items.size();
}

int  List::GetCurSel() const
{
	return _curSel;
}

void List::SetCurSel(int sel, bool scroll)
{
	_curSel = sel;
	if( scroll )
	{
		_scrollBar->SetPos((float) sel - 1);
	}
	if( eventChangeCurSel )
	{
		INVOKE(eventChangeCurSel) (sel);
	}
}

int  List::HitTest(float y)
{
    int index = int( y / _blankText->GetHeight() + _scrollBar->GetPos() );
	if( index < 0 || index >= (int) _items.size() )
	{
		index = -1;
	}
	return index;
}

void List::OnSize(float width, float height)
{
	_selection->Resize(width, _blankText->GetHeight()+2);
}

bool List::OnMouseDown(float x, float y, int button)
{
	if( 1 == button )
	{
		int index = HitTest(y);
		SetCurSel(index, false);
		if( -1 != index && eventClickItem ) 
			INVOKE(eventClickItem) (index);
	}
	return true;
}

bool List::OnMouseWheel(float x, float y, float z)
{
	_scrollBar->SetPos( _scrollBar->GetPos() - z * 3.0f );
	UpdateSelection();
	return true;
}

void List::OnRawChar(int c)
{
	switch(c)
	{
	case VK_UP:
		SetCurSel(__max(0, GetCurSel() - 1), false);
		break;
	case VK_DOWN:
		SetCurSel(__min(GetSize() - 1, GetCurSel() + 1), false);
		break;
	case VK_HOME:
		SetCurSel(0, false);
		break;
	case VK_END:
		SetCurSel(GetSize() - 1, false);
		break;
	default:
		GetParent()->OnRawChar(c);
	}
}

bool List::OnFocus(bool focus)
{
	return true;
}

void List::DrawChildren(float sx, float sy)
{
	UpdateSelection();

	Window::DrawChildren(sx, sy);

	size_t i_min = (size_t) _scrollBar->GetPos();
	size_t i_max = i_min + (size_t) (GetHeight() / _blankText->GetHeight()) + 2;

	_blankText->Show(true);
    for( size_t i = i_min; i < __min(_items.size(), i_max); ++i )
	{
		SpriteColor c = 0xc0c0c0c0;
		float c1 = 192;
		if( _curSel == i )
		{
			c1 = 255;
			c  = 0xffffffff;
		}

		float y = (float) i - _scrollBar->GetPos();

		_blankText->SetColor(c);

		y *= _blankText->GetHeight();
		y  = floorf(y + 0.5f);
		for( size_t k = 0; k < _items[i].text.size(); ++k )
		{
			_blankText->SetText(_items[i].text[k].c_str());
			_blankText->Draw(sx + _tabs[__min(k, _tabs.size()-1)], sy + y );
		}
	}
	_blankText->Show(false);
}


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

