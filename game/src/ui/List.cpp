// List.cpp

#include <ui/List.h>
#include <ui/Scroll.h>
#include <ui/GuiManager.h>
#include <video/TextureManager.h>
#include <video/DrawingContext.h>
#include <GLFW/glfw3.h>

#include <algorithm>

namespace UI
{

///////////////////////////////////////////////////////////////////////////////
// class List::ListCallbackImpl

List::ListCallbackImpl::ListCallbackImpl(List *list)
  : _list(list)
{
	assert(_list);
}

void List::ListCallbackImpl::OnDeleteAllItems()
{
	_list->_scrollBar->SetDocumentSize(0);
	_list->SetCurSel(-1, false);
}

void List::ListCallbackImpl::OnDeleteItem(int index)
{
	_list->_scrollBar->SetDocumentSize((float) _list->_data->GetItemCount());
	if( -1 != _list->GetCurSel() )
	{
		if( _list->GetCurSel() > index )
		{
			_list->SetCurSel(_list->GetCurSel() - 1, false);
		}
		else if( _list->GetCurSel() == index )
		{
			_list->SetCurSel(-1, false);
		}
	}
}

void List::ListCallbackImpl::OnAddItem()
{
	_list->_scrollBar->SetDocumentSize((float) _list->_data->GetItemCount());
}

///////////////////////////////////////////////////////////////////////////////
// class List

List* List::Create(Window *parent, ListDataSource* dataSource, float x, float y, float width, float height)
{
	List *res = new List(parent, dataSource);
	res->Move(x, y);
	res->Resize(width, height);
	return res;
}

List::List(Window *parent, ListDataSource* dataSource)
    : Window(parent)
    , _callbacks(this)
    , _data(dataSource)
    , _scrollBar(ScrollBarVertical::Create(this, 0, 0, 0))
    , _curSel(-1)
    , _hotItem(-1)
    , _font(GetManager().GetTextureManager().FindSprite("font_small"))
    , _selection(GetManager().GetTextureManager().FindSprite("ui/listsel"))
{
	SetTexture("ui/list", false);
	SetDrawBorder(true);
	SetTabPos(0, 1); // first column

	_data->AddListener(&_callbacks);
	_scrollBar->SetDocumentSize((float) _data->GetItemCount());
	_scrollBar->SetLineSize(1);
}

List::~List()
{
}

ListDataSource* List::GetData() const
{
	return _data;
}

void List::SetTabPos(int index, float pos)
{
	assert(index >= 0);
	if( index >= (int) _tabs.size() )
		_tabs.insert(_tabs.end(), 1+index - _tabs.size(), pos);
	else
		_tabs[index] = pos;
}

float List::GetItemHeight() const
{
	return GetManager().GetTextureManager().GetFrameHeight(_font, 0);
}

int List::GetCurSel() const
{
	return _curSel;
}

void List::SetCurSel(int sel, bool scroll)
{
	if( 0 == _data->GetItemCount() )
	{
		sel = -1;
	}

	if( _curSel != sel )
	{
		_curSel = sel;
		if( scroll )
		{
			float fs = (float) sel;
			if( fs < GetScrollPos() )
				SetScrollPos(fs);
			else if( fs > GetScrollPos() + GetNumLinesVisible() - 1 )
				SetScrollPos(fs - GetNumLinesVisible() + 1);
		}

		if( eventChangeCurSel )
			eventChangeCurSel(sel);
	}
}

int List::HitTest(float y)
{
	int index = int(y / GetItemHeight() + GetScrollPos());
	if( index < 0 || index >= _data->GetItemCount() )
	{
		index = -1;
	}
	return index;
}

float List::GetNumLinesVisible() const
{
	return GetHeight() / GetItemHeight();
}

float List::GetScrollPos() const
{
	return _scrollBar->GetPos();
}

void List::SetScrollPos(float line)
{
	_scrollBar->SetPos(line);
}

void List::AlignHeightToContent(float maxHeight)
{
	Resize(GetWidth(), std::min(maxHeight, GetItemHeight() * (float) _data->GetItemCount()));
}

void List::OnSize(float width, float height)
{
	_scrollBar->Resize(_scrollBar->GetWidth(), height);
	_scrollBar->Move(width - _scrollBar->GetWidth(), 0);
	_scrollBar->SetPageSize(GetNumLinesVisible());
}

bool List::OnMouseMove(float x, float y)
{
	_hotItem = HitTest(y);
	return true;
}

bool List::OnMouseLeave()
{
	_hotItem = -1;
	return true;
}

bool List::OnMouseDown(float x, float y, int button)
{
	if( 1 == button && x < _scrollBar->GetX() )
	{
		int index = HitTest(y);
		SetCurSel(index, false);
		if( -1 != index && eventClickItem )
			eventClickItem(index);
	}
	return true;
}

bool List::OnMouseUp(float x, float y, int button)
{
	return true;
}

bool List::OnMouseWheel(float x, float y, float z)
{
	SetScrollPos(GetScrollPos() - z * 3.0f);
	return true;
}

bool List::OnRawChar(int c)
{
	switch( c )
	{
	case GLFW_KEY_UP:
        SetCurSel(std::max(0, GetCurSel() - 1), true);
		break;
	case GLFW_KEY_DOWN:
        SetCurSel(std::min(_data->GetItemCount() - 1, GetCurSel() + 1), true);
		break;
	case GLFW_KEY_HOME:
		SetCurSel(0, true);
		break;
	case GLFW_KEY_END:
		SetCurSel(_data->GetItemCount() - 1, true);
		break;
	case GLFW_KEY_PAGE_UP:
        SetCurSel(std::max(0, GetCurSel() - (int) ceil(GetNumLinesVisible()) + 1), true);
		break;
	case GLFW_KEY_PAGE_DOWN:
        SetCurSel(std::min(_data->GetItemCount() - 1, GetCurSel() + (int) ceil(GetNumLinesVisible()) - 1), true);
		break;
	default:
		return false;
	}
	return true;
}

bool List::OnFocus(bool focus)
{
	return true;
}

void List::DrawChildren(DrawingContext &dc, float sx, float sy) const
{
	Window::DrawChildren(dc, sx, sy);

	float pos = GetScrollPos();
	int i_min = (int) pos;
	int i_max = i_min + (int) GetNumLinesVisible() + 1;
	int maxtab = (int) _tabs.size() - 1;

	Rect clip;
	clip.left   = (int) sx;
	clip.top    = (int) sy;
	clip.right  = (int) (sx + _scrollBar->GetX());
	clip.bottom = (int) (sy + GetHeight());
	dc.PushClippingRect(clip);

	for( int i = std::min(_data->GetItemCount(), i_max)-1; i >= i_min; --i )
	{
		SpriteColor c;
		float y = floorf(((float) i - pos) * GetItemHeight() + 0.5f);

		if( GetEnabled() )
		{
			c = 0xffd0d0d0; // normal;
			if( _curSel == i )
			{
				// selection frame around selected item
				FRECT sel = {sx + 1, sy + y, sx + GetWidth(), sy + y + GetItemHeight()};
				if( this == GetManager().GetFocusWnd() )
				{
					c = 0xff000000; // selected focused;
					dc.DrawSprite(&sel, _selection, 0xffffffff, 0);
				}
				else
				{
					c = 0xffffffff; // selected unfocused;
				}
				dc.DrawBorder(&sel, _selection, 0xffffffff, 0);
			}
			else if( _hotItem == i )
			{
				c = 0xffffffff; // hot;
			}
		}
		else
		{
			c = 0x70707070; // disabled;
		}

		for( int k = _data->GetSubItemCount(i); k--; )
		{
			dc.DrawBitmapText(sx + _tabs[std::min(k, maxtab)], sy + y, _font, c, _data->GetItemText(i, k));
		}
	}

	dc.PopClippingRect();
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
