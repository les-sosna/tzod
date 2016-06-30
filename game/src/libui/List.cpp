#include "inc/ui/InputContext.h"
#include "inc/ui/List.h"
#include "inc/ui/Scroll.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/Keys.h"
#include "inc/ui/LayoutContext.h"
#include <video/TextureManager.h>
#include <video/DrawingContext.h>

#include <algorithm>

using namespace UI;

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

List::List(LayoutManager &manager, TextureManager &texman, ListDataSource* dataSource)
    : Window(manager)
    , _callbacks(this)
    , _data(dataSource)
    , _scrollBar(std::make_shared<ScrollBarVertical>(manager, texman))
    , _curSel(-1)
    , _font(texman.FindSprite("font_small"))
    , _selection(texman.FindSprite("ui/listsel"))
{
	AddFront(_scrollBar);

	SetTexture(texman, "ui/list", false);
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

int List::HitTest(float y) const
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

bool List::OnPointerDown(InputContext &ic, vec2d size, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID)
{
	if( 1 == button && pointerPosition.x < _scrollBar->GetX() )
	{
		OnTap(ic, size, pointerPosition);
	}
	return false;
}

void List::OnMouseWheel(InputContext &ic, vec2d size, vec2d pointerPosition, float z)
{
	SetScrollPos(GetScrollPos() - z * 3.0f);
}

void List::OnTap(InputContext &ic, vec2d size, vec2d pointerPosition)
{
	int index = HitTest(pointerPosition.y);
	SetCurSel(index, false);
	if( -1 != index && eventClickItem )
		eventClickItem(index);
}

bool List::OnKeyPressed(InputContext &ic, Key key)
{
	switch( key )
	{
	case Key::Up:
		SetCurSel(std::max(0, GetCurSel() - 1), true);
		break;
	case Key::Down:
		SetCurSel(std::min(_data->GetItemCount() - 1, GetCurSel() + 1), true);
		break;
	case Key::Home:
		SetCurSel(0, true);
		break;
	case Key::End:
		SetCurSel(_data->GetItemCount() - 1, true);
		break;
	case Key::PageUp:
		SetCurSel(std::max(0, GetCurSel() - (int) ceil(GetNumLinesVisible()) + 1), true);
		break;
	case Key::PageDown:
		SetCurSel(std::min(_data->GetItemCount() - 1, GetCurSel() + (int) ceil(GetNumLinesVisible()) - 1), true);
		break;
	default:
		return false;
	}
	return true;
}

void List::Draw(const LayoutContext &lc, InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	Window::Draw(lc, ic, dc, texman);

	float pos = GetScrollPos();
	int i_min = (int) pos;
	int i_max = i_min + (int) (lc.GetPixelSize().y / GetItemHeight()) + 1;
	int maxtab = (int) _tabs.size() - 1;

	RectRB clip;
	clip.left = 0;
	clip.top = 0;
	clip.right = (int) (lc.GetPixelSize().x);
	clip.bottom = (int) (lc.GetPixelSize().y);
	dc.PushClippingRect(clip);

	int hotItem = ic.GetHovered() ? HitTest(ic.GetMousePos().y) : -1;

	for( int i = std::min(_data->GetItemCount(), i_max)-1; i >= i_min; --i )
	{
		SpriteColor c;
		float y = floorf(((float) i - pos) * GetItemHeight() + 0.5f);

		if( lc.GetEnabled() )
		{
			c = 0xffd0d0d0; // normal
			if( _curSel == i )
			{
				// selection frame around selected item
				if( ic.GetFocused() )
				{
					c = 0xff000000; // selected focused
					FRECT sel = { 1, y, lc.GetPixelSize().x - 1, y + GetItemHeight() };
					dc.DrawSprite(sel, _selection, 0xffffffff, 0);
				}
				else
				{
					c = 0xffffffff; // selected unfocused
				}
				FRECT border = { -1, y - 2, lc.GetPixelSize().x + 1, y + GetItemHeight() + 2 };
				dc.DrawBorder(border, _selection, 0xffffffff, 0);
			}
			else if( hotItem == i )
			{
				c = 0xffffffff; // hot
			}
		}
		else
		{
			c = 0x70707070; // disabled
		}

		for( int k = _data->GetSubItemCount(i); k--; )
		{
			float x = std::floor(_tabs[std::min(k, maxtab)] * lc.GetScale());
			dc.DrawBitmapText(x, y, _font, c, _data->GetItemText(i, k));
		}
	}

	dc.PopClippingRect();
}
