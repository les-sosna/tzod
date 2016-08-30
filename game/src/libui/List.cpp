#include "inc/ui/InputContext.h"
#include "inc/ui/List.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/Keys.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/StateContext.h"
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
	_list->SetCurSel(-1, false);
}

void List::ListCallbackImpl::OnDeleteItem(int index)
{
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
}

///////////////////////////////////////////////////////////////////////////////
// class List

List::List(LayoutManager &manager, TextureManager &texman, ListDataSource* dataSource)
    : Window(manager)
    , _callbacks(this)
    , _data(dataSource)
    , _curSel(-1)
    , _font(texman.FindSprite("font_small"))
    , _selection(texman.FindSprite("ui/listsel"))
{
	SetTabPos(0, 1); // first column
	_data->AddListener(&_callbacks);
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

void List::SetItemTemplate(std::shared_ptr<Window> itemTemplate)
{
	_itemTemplate = itemTemplate;
}

vec2d List::GetItemSize(TextureManager &texman, float scale) const
{
	if (_data->GetItemCount() > 0)
	{
		StateContext sc;
		sc.SetDataContext(_data);

		return _itemTemplate ? Vec2dFloor(_itemTemplate->GetContentSize(sc, texman) * scale) :
			vec2d{ 0.f, std::ceil(GetManager().GetTextureManager().GetFrameHeight(_font, 0) * scale) };
	}
	else
	{
		return vec2d{};
	}
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
			//if( fs < GetScrollPos() )
			//	SetScrollPos(fs);
			//else if( fs > GetScrollPos() + GetNumLinesVisible() - 1 )
			//	SetScrollPos(fs - GetNumLinesVisible() + 1);
		}

		if( eventChangeCurSel )
			eventChangeCurSel(sel);
	}
}

int List::HitTest(vec2d pxPos, TextureManager &texman, float scale) const
{
	vec2d which = pxPos / GetItemSize(texman, scale);
	int index = _flowDirection == FlowDirection::Vertical ? int(which.y) : int(which.x);
	if( index < 0 || index >= _data->GetItemCount() )
	{
		index = -1;
	}
	return index;
}

bool List::OnPointerDown(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID)
{
	if( 1 == button )
	{
		OnTap(ic, lc, texman, pointerPosition);
	}
	return false;
}

void List::OnTap(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition)
{
	int index = HitTest(pointerPosition, texman, lc.GetScale());
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
	//case Key::PageUp:
	//	SetCurSel(std::max(0, GetCurSel() - (int) ceil(GetNumLinesVisible()) + 1), true);
	//	break;
	//case Key::PageDown:
	//	SetCurSel(std::min(_data->GetItemCount() - 1, GetCurSel() + (int) ceil(GetNumLinesVisible()) - 1), true);
	//	break;
	default:
		return false;
	}
	return true;
}

float List::GetWidth() const
{
	return _flowDirection == FlowDirection::Vertical ? 0.f : _itemTemplate->GetWidth() * _data->GetItemCount();
}

float List::GetHeight() const
{
	return _flowDirection == FlowDirection::Vertical ? /*GetItemSize(1.f).y*/33 * _data->GetItemCount() : 0.f;
}

void List::Draw(const StateContext &sc, const LayoutContext &lc, const InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	RectRB visibleRegion = dc.GetVisibleRegion();

	bool isVertical = _flowDirection == FlowDirection::Vertical;

	vec2d pxItemMinSize = GetItemSize(texman, lc.GetScale());

	vec2d pxItemSize = isVertical ?
		vec2d{ lc.GetPixelSize().x, pxItemMinSize.y } : vec2d{ pxItemMinSize.x, lc.GetPixelSize().y };

	int advance = isVertical ? (int)pxItemSize.y : (int)pxItemSize.x;

	if (advance == 0) // cannot draw zero-sized elements
		return;

	int regionBegin = isVertical ? visibleRegion.top : visibleRegion.left;
	int regionEnd = isVertical ? visibleRegion.bottom : visibleRegion.right;

	int i_min = std::max(0, regionBegin / advance);
	int i_max = std::max(0, (regionEnd + advance - 1) / advance);
	int maxtab = (int) _tabs.size() - 1;

	int hotItem = ic.GetHovered() ? HitTest(ic.GetMousePos(), texman, lc.GetScale()) : -1;

	for( int i = std::min(_data->GetItemCount(), i_max)-1; i >= i_min; --i )
	{
		SpriteColor c;
		vec2d pxItemOffset = isVertical ?
			vec2d{ 0, (float)i * pxItemSize.y } : vec2d{ (float)i * pxItemSize.x, 0 };

		enum ItemState { NORMAL, UNFOCUSED, FOCUSED, HOVER, DISABLED } itemState;
		if (lc.GetEnabled())
		{
			if (_curSel == i)
			{
				if (ic.GetFocused())
					itemState = FOCUSED;
				else
					itemState = UNFOCUSED;
			}
			else if (hotItem == i)
				itemState = HOVER;
			else
				itemState = NORMAL;
		}
		else
			itemState = DISABLED;

		if (_itemTemplate)
		{
			// TODO: something smarter than const_cast (fork?)
			UI::RenderSettings rs{ const_cast<StateContext&>(sc), const_cast<LayoutContext&>(lc), const_cast<InputContext&>(ic), dc, texman };

			static const char* itemStateStrings[] = { "Normal", "Unfocused", "Focused", "Hover", "Disabled" };
			rs.sc.SetState(itemStateStrings[itemState]);
			rs.sc.SetDataContext(_data);
			rs.sc.SetItemIndex(i);

			rs.lc.PushTransform(pxItemOffset, pxItemSize, true);
			rs.ic.PushTransform(pxItemOffset, true, true);
			dc.PushTransform(pxItemOffset);

			RenderUIRoot(*_itemTemplate, rs);

			dc.PopTransform();
			rs.ic.PopTransform();
			rs.lc.PopTransform();
		}
		else
		{
			static SpriteColor itemStateColors[] =
			// Normal      Unfocused   Focused     Hover        Disabled
			{ 0xffd0d0d0, 0xffffffff, 0xff000000, 0xffffffff,  0x70707070};

			c = itemStateColors[itemState];
			FRECT sel = { 1, pxItemOffset.y, lc.GetPixelSize().x - 1, pxItemOffset.y + pxItemSize.y };
			FRECT border = { -1, pxItemOffset.y - 2, lc.GetPixelSize().x + 1, pxItemOffset.y + pxItemSize.y + 2 };

			switch (itemState)
			{
			case FOCUSED:
				dc.DrawSprite(sel, _selection, 0xffffffff, 0);
				// fall through
			case UNFOCUSED:
				dc.DrawBorder(border, _selection, 0xffffffff, 0);
				break;
			default:
				break;
			}

			for (int k = _data->GetSubItemCount(i); k--; )
			{
				float x = std::floor(_tabs[std::min(k, maxtab)] * lc.GetScale());
				dc.DrawBitmapText(x, pxItemOffset.y, _font, c, _data->GetItemText(i, k));
			}
		}
	}
}
