// List.cpp

#include "stdafx.h"

#include "List.h"
#include "Scroll.h"

#include "video/TextureManager.h"

namespace UI
{

///////////////////////////////////////////////////////////////////////////////
// class ListDataSourceDefault

void ListDataSourceDefault::SetListener(ListDataSourceListener *cb)
{
	_listener = cb;
}

int ListDataSourceDefault::GetItemCount() const
{
	return (int) _items.size();
}

int ListDataSourceDefault::GetSubItemCount(int index) const
{
	return _items[index].text.size();
}

ULONG_PTR ListDataSourceDefault::GetItemData(int index) const
{
	assert(index >= 0);
	return _items[index].data;
}

const string_t& ListDataSourceDefault::GetItemText(int index, int sub) const
{
	assert(index >= 0 && index < (int) _items.size());
	assert(sub >= 0 && sub < (int) _items[index].text.size());
	return _items[index].text[sub];
}

int ListDataSourceDefault::FindItem(const string_t &text) const
{
	for( size_t i = 0; i < _items.size(); ++i )
	{
		if( _items[i].text[0] == text )
		{
			return i;
		}
	}
	return -1;
}

int ListDataSourceDefault::AddItem(const string_t &str, UINT_PTR data)
{
	Item i;
	i.text.push_back(str);
	i.data = data;
	_items.push_back(i);

	if( _listener )
		_listener->OnAddItem();

	return _items.size() - 1;
}

void ListDataSourceDefault::SetItemText(int index, int sub, const string_t &str)
{
	assert(index >= 0 && index < (int) _items.size());
	if( sub >= (int) _items[index].text.size() )
		_items[index].text.insert(_items[index].text.end(), 1+sub - _items[index].text.size(), "");
	_items[index].text[sub] = str;
}

void ListDataSourceDefault::SetItemData(int index, ULONG_PTR data)
{
	assert(index >= 0);
	_items[index].data = data;
}

void ListDataSourceDefault::DeleteItem(int index)
{
	_items.erase(_items.begin() + index);
	if( _listener )
		_listener->OnDeleteItem(index);
}

void ListDataSourceDefault::DeleteAllItems()
{
	_items.clear();
	if( _listener )
		_listener->OnDeleteAllItems();
}

void ListDataSourceDefault::Sort()
{
	struct helper
	{
		static bool compare(const Item &left, const Item &right)
		{
			return left.text < right.text;
		}
	};
	std::sort(_items.begin(), _items.end(), &helper::compare);
}


///////////////////////////////////////////////////////////////////////////////
// class List::ListCallbackImpl

List::ListCallbackImpl::ListCallbackImpl(List *list)
  : _list(list)
{
	assert(_list);
}

void List::ListCallbackImpl::OnDeleteAllItems()
{
	_list->_scrollBar->SetLimit(-1);
	_list->SetCurSel(-1, false);
}

void List::ListCallbackImpl::OnDeleteItem(int idx)
{
	_list->_scrollBar->SetLimit((float) _list->_data->GetItemCount() - _list->GetNumLinesVisible());
	if( -1 != _list->GetCurSel() )
	{
		if( _list->GetCurSel() > idx )
		{
			_list->SetCurSel(_list->GetCurSel() - 1, false);
		}
		else if( _list->GetCurSel() == idx )
		{
			_list->SetCurSel(-1, false);
		}
	}
}

void List::ListCallbackImpl::OnAddItem()
{
	_list->_scrollBar->SetLimit((float) _list->_data->GetItemCount() - _list->GetNumLinesVisible());
}

///////////////////////////////////////////////////////////////////////////////
// class List

List::List(Window *parent, float x, float y, float width, float height)
  : Window(parent, x, y, "ui/list")
  , _callbacks(this)
  , _curSel(-1)
  , _hotItem(-1)
  , _font(g_texman->FindSprite("font_small"))
{
	SetClipChildren(true);
	SetBorder(true);

	SetData(NULL);
	SetTabPos(0, 1); // first column

	_scrollBar = new ScrollBar(this, 0, 0, height);
	_scrollBar->Move(width - _scrollBar->GetWidth(), 0);
	_scrollBar->eventScroll.bind(&List::OnScroll, this);

	_selection = new Window(this, 0, 0, "ui/listsel_u");
	_selection->SetBorder(true);
	_selection->Resize(1, GetItemHeight());

	Move(x, y);
	Resize(width, height); // it will resize the selection also, so create it first!
}

List::~List()
{
}

SafePtr<ListDataSource> List::GetData() const
{
	return _data;
}

SafePtr<ListDataSourceDefault> List::GetDataDefault() const
{
	return SafePtrCast<ListDataSourceDefault>(_data);
}

void List::SetData(const SafePtr<ListDataSource> &source)
{
	_data = source ? source : WrapRawPtr(new ListDataSourceDefault());
	_data->SetListener(&_callbacks);
}

void List::OnScroll(float pos)
{
	UpdateSelection();
}

void List::UpdateSelection()
{
	float y = (float) _curSel - _scrollBar->GetPos();
	_selection->Move(2, floorf(y * GetItemHeight()));
	_selection->SetVisible(-1 != _curSel && y > -1 && y < GetNumLinesVisible());
}

void List::DeleteItem(int index)
{
	GetDataDefault()->DeleteItem(index);
}

void List::DeleteAllItems()
{
	GetDataDefault()->DeleteAllItems();
}

int List::AddItem(const string_t &str, UINT_PTR data)
{
	return GetDataDefault()->AddItem(str, data);
}

void List::SetItemText(int index, int sub, const string_t &str)
{
	GetDataDefault()->SetItemText(index, sub, str);
}

void List::SetItemData(int index, ULONG_PTR data)
{
	GetDataDefault()->SetItemData(index, data);
}

void List::Sort()
{
	GetDataDefault()->Sort();
}

int List::FindItem(const string_t &text) const
{
	return _data->FindItem(text);
}

int List::GetItemCount() const
{
	return _data->GetItemCount();
}

const string_t& List::GetItemText(int index, int sub) const
{
	return _data->GetItemText(index, sub);
}

ULONG_PTR List::GetItemData(int index) const
{
	return _data->GetItemData(index);
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
	return g_texman->Get(_font).pxFrameHeight + 1;
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
			if( fs < _scrollBar->GetPos() )
				_scrollBar->SetPos(fs);
			else if( fs > _scrollBar->GetPos() + GetNumLinesVisible() - 1 )
				_scrollBar->SetPos(fs - GetNumLinesVisible() + 1);
		}

		UpdateSelection();

		if( eventChangeCurSel )
		{
			INVOKE(eventChangeCurSel) (sel);
		}
	}
}

int List::HitTest(float y)
{
	int index = int(y / GetItemHeight() + _scrollBar->GetPos());
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

void List::SetScrollPos(float pos)
{
	_scrollBar->SetPos(pos);
	UpdateSelection();
}

void List::AlignHeightToContent(float maxHeight)
{
	Resize(GetWidth(), __min(maxHeight, GetItemHeight() * (float) _data->GetItemCount()));
}

void List::OnSize(float width, float height)
{
	_selection->Resize(width, _selection->GetHeight());
	_scrollBar->Resize(_scrollBar->GetWidth(), height);
	_scrollBar->Move(width - _scrollBar->GetWidth(), 0);
	_scrollBar->SetLimit( (float) _data->GetItemCount() - GetNumLinesVisible() );
	UpdateSelection();
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
			INVOKE(eventClickItem) (index);
	}
	return true;
}

bool List::OnMouseWheel(float x, float y, float z)
{
	_scrollBar->SetPos(_scrollBar->GetPos() - z * 3.0f);
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
		SetCurSel(__min(_data->GetItemCount() - 1, GetCurSel() + 1), true);
		break;
	case VK_HOME:
		SetCurSel(0, true);
		break;
	case VK_END:
		SetCurSel(_data->GetItemCount() - 1, true);
		break;
	case VK_PRIOR: // page up
		SetCurSel(__max(0, GetCurSel() - (int) ceil(GetNumLinesVisible()) + 1), true);
		break;
	case VK_NEXT:  // page down
		SetCurSel(__min(_data->GetItemCount() - 1, GetCurSel() + (int) ceil(GetNumLinesVisible()) - 1), true);
		break;
	default:
		GetParent()->OnRawChar(c);
	}
}

bool List::OnFocus(bool focus)
{
	_selection->SetTexture(focus ? "ui/listsel" : "ui/listsel_u");
	return true;
}

void List::DrawChildren(float sx, float sy) const
{
	Window::DrawChildren(sx, sy);

	int i_min = (int) _scrollBar->GetPos();
	int i_max = i_min + (int) GetNumLinesVisible() + 2;

	for( int i = i_min; i < std::min(_data->GetItemCount(), i_max); ++i )
	{
		SpriteColor c = 0xc0c0c0c0;
		if( _hotItem == i )
		{
			c  = 0xffccccff;
		}
		else if( _curSel == i )
		{
			c  = 0xffffffff;
		}

		float y = floorf(GetItemHeight() * ((float) i - _scrollBar->GetPos()) + 0.5f);
		for( int k = 0; k < _data->GetSubItemCount(i); ++k )
		{
			g_texman->DrawBitmapText(_font, _data->GetItemText(i, k), c, 
				sx + _tabs[__min(k, (int) _tabs.size()-1)], sy + y);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
