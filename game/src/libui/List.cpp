#include "inc/ui/DataContext.h"
#include "inc/ui/InputContext.h"
#include "inc/ui/List.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/Keys.h"
#include "inc/ui/LayoutContext.h"
#include "inc/ui/StateContext.h"
#include <video/TextureManager.h>
#include <video/RenderContext.h>

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
	_list->SetCurSel(-1);
}

void List::ListCallbackImpl::OnDeleteItem(int index)
{
	if( -1 != _list->GetCurSel() )
	{
		if( _list->GetCurSel() > index )
		{
			_list->SetCurSel(_list->GetCurSel() - 1);
		}
		else if( _list->GetCurSel() == index )
		{
			_list->SetCurSel(-1);
		}
	}
}

void List::ListCallbackImpl::OnAddItem()
{
}

///////////////////////////////////////////////////////////////////////////////
// class List

List::List(ListDataSource* dataSource)
	: _callbacks(this)
	, _data(dataSource)
	, _curSel(-1)
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
		DataContext dc;
		dc.SetDataContext(_data);

		return _itemTemplate->GetContentSize(texman, dc, scale);
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

void List::SetCurSel(int sel)
{
	if( 0 == _data->GetItemCount() )
	{
		sel = -1;
	}

	if( _curSel != sel )
	{
		_curSel = sel;
//		if( scroll )
//		{
//			float fs = (float) sel;
//			if( fs < GetScrollPos() )
//				SetScrollPos(fs);
//			else if( fs > GetScrollPos() + GetNumLinesVisible() - 1 )
//				SetScrollPos(fs - GetNumLinesVisible() + 1);
//		}

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

bool List::OnPointerDown(InputContext &ic, LayoutContext &lc, TextureManager &texman, PointerInfo pi, int button)
{
	if( 1 == button && pi.type == PointerType::Mouse )
	{
		OnTap(ic, lc, texman, pi.position);
	}
	return false;
}

void List::OnTap(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition)
{
	int index = HitTest(pointerPosition, texman, lc.GetScale());
	SetCurSel(index);
	if( -1 != index && eventClickItem )
		eventClickItem(index);
}

int List::GetNextIndex(Navigate navigate) const
{
	int maxIndex = _data->GetItemCount() - 1;
	switch (navigate)
	{
	case UI::Navigate::Up:
		return std::min(maxIndex, std::max(0, GetCurSel() - (_flowDirection == FlowDirection::Vertical)));
	case UI::Navigate::Down:
		return std::min(maxIndex, std::max(0, GetCurSel() + (_flowDirection == FlowDirection::Vertical)));
	case UI::Navigate::Left:
		return std::min(maxIndex, std::max(0, GetCurSel() - (_flowDirection == FlowDirection::Horizontal)));
	case UI::Navigate::Right:
		return std::min(maxIndex, std::max(0, GetCurSel() + (_flowDirection == FlowDirection::Horizontal)));
	case UI::Navigate::Begin:
		return std::min(maxIndex, 0);
	case UI::Navigate::End:
		return maxIndex;
	default:
		return GetCurSel();
	}
}

bool List::CanNavigate(Navigate navigate, const DataContext &dc) const
{
	return GetNextIndex(navigate) != GetCurSel();
}

void List::OnNavigate(Navigate navigate, NavigationPhase phase, const DataContext &dc)
{
	if (NavigationPhase::Started == phase)
	{
		SetCurSel(GetNextIndex(navigate));
	}
}

vec2d List::GetContentSize(TextureManager &texman, const DataContext &dc, float scale) const
{
	vec2d pxItemSize = GetItemSize(texman, scale);
	return _flowDirection == FlowDirection::Vertical ?
		vec2d{ pxItemSize.x, pxItemSize.y * _data->GetItemCount() } :
		vec2d{ pxItemSize.x * _data->GetItemCount(), pxItemSize.y };
}

void List::Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const
{
	if (!_itemTemplate)
		return;

	RectRB visibleRegion = rc.GetVisibleRegion();

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

		InputContext childIC(ic);
		UI::RenderSettings rs{ childIC, rc, texman, time };

		StateContext itemSC;
		{
			static const char* itemStateStrings[] = { "Normal", "Unfocused", "Focused", "Hover", "Disabled" };
			itemSC.SetState(itemStateStrings[itemState]);
		}

		DataContext itemDC;
		{
			itemDC.SetDataContext(_data);
			itemDC.SetItemIndex(i);
		}

		rs.ic.PushInputTransform(pxItemOffset, true, true);
		rc.PushTransform(pxItemOffset, lc.GetOpacityCombined());

		LayoutContext itemLC(lc.GetOpacityCombined(), lc.GetScale(), pxItemSize, lc.GetEnabledCombined());
		RenderUIRoot(*_itemTemplate, rs, itemLC, itemDC, itemSC);

		rc.PopTransform();
		rs.ic.PopInputTransform();
	}
}
