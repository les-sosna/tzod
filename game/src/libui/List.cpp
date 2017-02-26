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
	_data->AddListener(&_callbacks);
}

List::~List()
{
}

ListDataSource* List::GetData() const
{
	return _data;
}

void List::SetItemTemplate(std::shared_ptr<Window> itemTemplate)
{
	_itemTemplate = itemTemplate;
}

vec2d List::GetItemSize(TextureManager &texman, float scale) const
{
	if (_itemTemplate && _data->GetItemCount() > 0)
	{
		StateContext sc;
		sc.SetDataContext(_data);

		return _itemTemplate->GetContentSize(texman, sc, scale);
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
    if( 1 == button && pointerType == PointerType::Mouse )
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
		if (_flowDirection == FlowDirection::Vertical)
			SetCurSel(std::max(0, GetCurSel() - 1), true);
		break;
	case Key::Left:
		if (_flowDirection == FlowDirection::Horizontal)
			SetCurSel(std::max(0, GetCurSel() - 1), true);
		break;
	case Key::Down:
		if (_flowDirection == FlowDirection::Vertical)
			SetCurSel(std::min(_data->GetItemCount() - 1, GetCurSel() + 1), true);
		break;
	case Key::Right:
		if (_flowDirection == FlowDirection::Horizontal)
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

vec2d List::GetContentSize(TextureManager &texman, const StateContext &sc, float scale) const
{
	vec2d pxItemSize = GetItemSize(texman, scale);
	return _flowDirection == FlowDirection::Vertical ?
		vec2d{ pxItemSize.x, pxItemSize.y * _data->GetItemCount() } :
		vec2d{ pxItemSize.x * _data->GetItemCount(), pxItemSize.y };
}

void List::Draw(const StateContext &sc, const LayoutContext &lc, const InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	if (!_itemTemplate)
		return;

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

	int hotItem = ic.GetHovered() ? HitTest(ic.GetMousePos(), texman, lc.GetScale()) : -1;

	for( int i = std::min(_data->GetItemCount(), i_max)-1; i >= i_min; --i )
	{
		vec2d pxItemOffset = isVertical ?
			vec2d{ 0, (float)i * pxItemSize.y } : vec2d{ (float)i * pxItemSize.x, 0 };

		enum ItemState { NORMAL, UNFOCUSED, FOCUSED, HOVER, DISABLED } itemState;
		if (lc.GetEnabledCombined())
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

		// TODO: something smarter than const_cast (fork?)
		UI::RenderSettings rs{ const_cast<InputContext&>(ic), dc, texman };

		StateContext itemSC;
		{
			static const char* itemStateStrings[] = { "Normal", "Unfocused", "Focused", "Hover", "Disabled" };
			itemSC.SetState(itemStateStrings[itemState]);
			itemSC.SetDataContext(_data);
			itemSC.SetItemIndex(i);
		}

		rs.ic.PushInputTransform(pxItemOffset, true, true);
		dc.PushTransform(pxItemOffset, lc.GetOpacityCombined());

		LayoutContext itemLC(lc.GetOpacityCombined(), lc.GetScale(), lc.GetPixelOffset() + pxItemOffset, pxItemSize, lc.GetEnabledCombined());
		RenderUIRoot(*_itemTemplate, rs, itemLC, itemSC);

		dc.PopTransform();
		rs.ic.PopInputTransform();
	}
}
