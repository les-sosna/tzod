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

	_scrollBar = new ScrollBar(this, 0, 0, height);
	_scrollBar->Move(width - _scrollBar->GetWidth(), 0);
	_scrollBar->eventScroll.bind(&List::OnScroll, this);

	_blankText = new Text(this, 0, 0, " ", alignTextLT);
	_blankText->Show(false);

	_selection = new Window(this, 0, 0, "ctrl_listsel_u");
	_selection->SetBorder(true);

	Resize(width, height); // it will resize the selection also, so create it first!
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
	_selection->Move(0, floorf(y * GetItemHeight()));
	_selection->Show( -1 != _curSel && y > -1 && y < GetNumLinesVisible() );
}

void List::DeleteAllItems()
{
	_items.clear();
	_scrollBar->SetLimit(-1);
	SetCurSel(-1, false);
}

int List::AddItem(const char *str, UINT_PTR data)
{
	Item i;
	i.text.push_back(str);
	i.data = data;
	_items.push_back(i);
	_scrollBar->SetLimit( (float) _items.size() - GetNumLinesVisible() );

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

float List::GetItemHeight() const
{
	return _blankText->GetHeight() + 1;
}

int List::GetSize() const
{
	return (int) _items.size();
}

int List::GetCurSel() const
{
	return _curSel;
}

void List::SetCurSel(int sel, bool scroll)
{
	_curSel = sel;
	if( scroll )
	{
		float fs = (float) sel;
		if( fs < _scrollBar->GetPos() )
			_scrollBar->SetPos(fs);
		else if( fs > _scrollBar->GetPos() + GetNumLinesVisible() - 1 )
			_scrollBar->SetPos(fs - GetNumLinesVisible() + 1);
	}
	if( eventChangeCurSel )
	{
		INVOKE(eventChangeCurSel) (sel);
	}
}

int List::HitTest(float y)
{
    int index = int( y / GetItemHeight() + _scrollBar->GetPos() );
	if( index < 0 || index >= (int) _items.size() )
	{
		index = -1;
	}
	return index;
}

float List::GetNumLinesVisible() const
{
	return GetHeight() / GetItemHeight();
}

void List::ScrollTo(float pos)
{
	_scrollBar->SetPos(pos);
}

void List::AlignHeightToContent()
{
	Resize(GetWidth(), GetItemHeight() * (float) GetSize());
}

void List::OnSize(float width, float height)
{
	_selection->Resize(width, _selection->GetHeight());
	_scrollBar->Resize(_scrollBar->GetWidth(), height);
	_scrollBar->Move(width - _scrollBar->GetWidth(), 0);
	_scrollBar->SetLimit( (float) _items.size() - GetNumLinesVisible() );
	UpdateSelection();
}

bool List::OnMouseDown(float x, float y, int button)
{
	if( 1 == button && x < _scrollBar->GetX() )
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
		SetCurSel(__max(0, GetCurSel() - 1), true);
		break;
	case VK_DOWN:
		SetCurSel(__min(GetSize() - 1, GetCurSel() + 1), true);
		break;
	case VK_HOME:
		SetCurSel(0, true);
		break;
	case VK_END:
		SetCurSel(GetSize() - 1, true);
		break;
	case VK_PRIOR: // page up
		SetCurSel(__max(0, GetCurSel() - (int) ceil(GetNumLinesVisible()) + 1), true);
		break;
	case VK_NEXT:  // page down
		SetCurSel(__min(GetSize() - 1, GetCurSel() + (int) ceil(GetNumLinesVisible()) - 1), true);
		break;
	default:
		GetParent()->OnRawChar(c);
	}
}

bool List::OnFocus(bool focus)
{
	_selection->SetTexture(focus ? "ctrl_listsel" : "ctrl_listsel_u");
	return true;
}

void List::DrawChildren(float sx, float sy)
{
	UpdateSelection();

	Window::DrawChildren(sx, sy);

	size_t i_min = (size_t) _scrollBar->GetPos();
	size_t i_max = i_min + (size_t) GetNumLinesVisible() + 2;

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

		y = floorf(y * GetItemHeight() + 0.5f);
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

