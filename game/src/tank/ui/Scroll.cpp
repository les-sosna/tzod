// Scroll.cpp

#include "stdafx.h"
#include "Scroll.h"
#include "Button.h"

namespace UI
{

///////////////////////////////////////////////////////////////////////////////
// scrollbar class implementation

ScrollBarBase::ScrollBarBase(Window *parent)
  : Window(parent)
  , _showButtons(true)
  , _pos(0)
  , _lineSize(0.1f)
  , _pageSize(0)
  , _limit(1.0f)
  , _tmpBoxPos(-1)
{
	_btnBox        = ImageButton::Create(this, 0, 0, NULL);
	_btnUpLeft     = ImageButton::Create(this, 0, 0, NULL);
	_btnDownRight  = ImageButton::Create(this, 0, 0, NULL);

	_btnUpLeft->eventClick.bind(&ScrollBarBase::OnUpLeft, this);
	_btnDownRight->eventClick.bind(&ScrollBarBase::OnDownRight, this);

	_btnBox->eventMouseUp.bind(&ScrollBarBase::OnBoxMouseUp, this);
	_btnBox->eventMouseDown.bind(&ScrollBarBase::OnBoxMouseDown, this);
	_btnBox->eventMouseMove.bind(&ScrollBarBase::OnBoxMouseMove, this);
	_btnBox->SetDrawBorder(true);

	SetDrawBorder(true);
	SetShowButtons(false);
}

void ScrollBarBase::SetShowButtons(bool showButtons)
{
	_showButtons = showButtons;
	_btnUpLeft->SetVisible(showButtons);
	_btnDownRight->SetVisible(showButtons);
	SetPos(GetPos());  // to update scroll box position
}

bool ScrollBarBase::GetShowButtons() const
{
	return _showButtons;
}

void ScrollBarBase::SetPos(float pos)
{
	_pos = __max(0, __min(_limit, pos));
}

float ScrollBarBase::GetPos() const
{
	return _pos;
}

void ScrollBarBase::SetPageSize(float ps)
{
	_pageSize = ps;
	OnLimitsChanged();
	SetPos(GetPos()); // update scroll box size and position
}

float ScrollBarBase::GetPageSize() const
{
	return _pageSize;
}

void ScrollBarBase::SetLimit(float limit)
{
	_limit = limit;
	OnLimitsChanged();
	SetPos(GetPos()); // update scroll box position
}

float ScrollBarBase::GetLimit() const
{
	return _limit;
}

void ScrollBarBase::SetLineSize(float ls)
{
	_lineSize = ls;
}

float ScrollBarBase::GetLineSize() const
{
	return _lineSize;
}

void ScrollBarBase::SetElementTextures(const char *slider, const char *upleft, const char *downright)
{
	_btnBox->SetTexture(slider, true);
	_btnUpLeft->SetTexture(upleft, true);
	_btnDownRight->SetTexture(downright, true);

	_btnBox->Move((GetWidth() - _btnBox->GetWidth()) / 2, (GetHeight() - _btnBox->GetHeight()) / 2);
	SetPos(GetPos()); // update scroll position
}

void ScrollBarBase::OnEnabledChange(bool enable, bool inherited)
{
	SetFrame(enable ? 0 : 1);
}

void ScrollBarBase::OnBoxMouseDown(float x, float y)
{
	_tmpBoxPos = Select(x, y);
}

void ScrollBarBase::OnBoxMouseUp(float x, float y)
{
	_tmpBoxPos = -1;
}

void ScrollBarBase::OnBoxMouseMove(float x, float y)
{
	if( _tmpBoxPos < 0 )
		return;
	float pos = Select(_btnBox->GetX() + x, _btnBox->GetY() + y) - _tmpBoxPos;
	if( _showButtons )
	{
		pos -= Select(_btnUpLeft->GetWidth(), _btnUpLeft->GetHeight());
	}
	pos /= GetScrollPaneLength() - Select(_btnBox->GetWidth(), _btnBox->GetHeight());
	SetPos(pos * _limit);
	if( eventScroll )
		INVOKE(eventScroll) (GetPos());
}

void ScrollBarBase::OnUpLeft()
{
	SetPos(GetPos() - _lineSize);
	if( eventScroll )
		INVOKE(eventScroll) (GetPos());
}

void ScrollBarBase::OnDownRight()
{
	SetPos(GetPos() + _lineSize);
	if( eventScroll )
		INVOKE(eventScroll) (GetPos());
}

void ScrollBarBase::OnLimitsChanged()
{
	bool needScroll = _limit > _pageSize;
	_btnBox->SetVisible(needScroll);
	_btnUpLeft->SetEnabled(needScroll);
	_btnDownRight->SetEnabled(needScroll);
	if( !needScroll )
	{
		SetPos(0);
	}
}

void ScrollBarBase::OnSize(float width, float height)
{
	_btnDownRight->Move( Select(width - _btnDownRight->GetWidth(), _btnDownRight->GetX()),
	                     Select(_btnDownRight->GetY(), height - _btnDownRight->GetHeight()) );
	SetPos(GetPos());  // to update scroll box position
}

float ScrollBarBase::GetScrollPaneLength() const
{
	float result = Select(GetWidth(), GetHeight());
	if( _showButtons )
	{
		result -= Select( _btnDownRight->GetWidth() + _btnUpLeft->GetWidth(),
		                 _btnDownRight->GetHeight() + _btnUpLeft->GetHeight() );
	}
	return result;
}

///////////////////////////////////////////////////////////////////////////////

ScrollBarVertical* ScrollBarVertical::Create(Window *parent, float x, float y, float height)
{
	ScrollBarVertical *result = new ScrollBarVertical(parent);
	result->Move(x, y);
	result->SetSize(height);
	return result;
}

ScrollBarVertical::ScrollBarVertical(Window *parent)
  : ScrollBarBase(parent)
{
	_btnBox->SetTexture("ui/scroll_vert", true);
	_btnUpLeft->SetTexture("ui/scroll_up", true);
	_btnDownRight->SetTexture("ui/scroll_down", true);
	SetTexture("ui/scroll_back_vert", true);
}

void ScrollBarVertical::SetSize(float size)
{
	Resize(GetWidth(), size);
}

float ScrollBarVertical::GetSize() const
{
	return GetHeight();
}

void ScrollBarVertical::SetPos(float pos)
{
	ScrollBarBase::SetPos(pos);

	float mult = GetShowButtons() ? 1.0f : 0.0f;
	_btnBox->Resize(_btnBox->GetWidth(),
		std::max(GetScrollPaneLength() * GetPageSize() / GetLimit(), _btnBox->GetTextureHeight()));
	_btnBox->Move(_btnBox->GetX(), floorf(_btnUpLeft->GetHeight() * mult + (GetHeight() - _btnBox->GetHeight()
		- (_btnDownRight->GetHeight() + _btnUpLeft->GetHeight()) * mult ) * GetPos() / GetLimit() + 0.5f));
}

///////////////////////////////////////////////////////////////////////////////

ScrollBarHorizontal* ScrollBarHorizontal::Create(Window *parent, float x, float y, float height)
{
	ScrollBarHorizontal *result = new ScrollBarHorizontal(parent);
	result->Move(x, y);
	result->SetSize(height);
	return result;
}

ScrollBarHorizontal::ScrollBarHorizontal(Window *parent)
  : ScrollBarBase(parent)
{
	_btnBox->SetTexture("ui/scroll_hor", true);
	_btnUpLeft->SetTexture("ui/scroll_left", true);
	_btnDownRight->SetTexture("ui/scroll_right", true);
	SetTexture("ui/scroll_back_hor", true);
}

void ScrollBarHorizontal::SetSize(float size)
{
	Resize(size, GetHeight());
}

float ScrollBarHorizontal::GetSize() const
{
	return GetWidth();
}

void ScrollBarHorizontal::SetPos(float pos)
{
	ScrollBarBase::SetPos(pos);

	float mult = GetShowButtons() ? 1.0f : 0.0f;
	_btnBox->Resize(std::max(GetScrollPaneLength() * GetPageSize() / GetLimit(), _btnBox->GetTextureWidth()),
		_btnBox->GetHeight());
	_btnBox->Move(floorf(_btnUpLeft->GetWidth() * mult + (GetWidth() - _btnBox->GetWidth()
		- (_btnUpLeft->GetWidth() + _btnDownRight->GetWidth()) * mult) * GetPos() / GetLimit() + 0.5f), _btnBox->GetY());
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
