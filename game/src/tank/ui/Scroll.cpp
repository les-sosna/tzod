// Scroll.cpp

#include "stdafx.h"
#include "Scroll.h"
#include "Button.h"

namespace UI
{
;
///////////////////////////////////////////////////////////////////////////////
// scrollbar class implementation

ScrollBar::ScrollBar(Window *parent, float x, float y, float height)
  : Window(parent)
{
	_pos       = 0;
	_lineSize  = 1.0f;
	_limit     = 1.0f;
	_tmpBoxY   = -1;

    _btnUp    = new ImageButton(this, 0, 0, "ctrl_scroll_up");
    _btnBox   = new ImageButton(this, 0, 0, "ctrl_scroll_box");
    _btnDown  = new ImageButton(this, 0, 0, "ctrl_scroll_down");

	_btnUp->eventClick.bind(&ScrollBar::OnUp, this);
	_btnDown->eventClick.bind(&ScrollBar::OnDown, this);

	_btnBox->eventMouseDown.bind(&ScrollBar::OnBoxMouseDown, this);
	_btnBox->eventMouseUp.bind(  &ScrollBar::OnBoxMouseUp,   this);
	_btnBox->eventMouseMove.bind(&ScrollBar::OnBoxMouseMove, this);

	Resize(_btnUp->GetWidth(), height);
	Move(x, y);
}

void  ScrollBar::SetPos(float pos)
{
	_pos = __max(0, __min(_limit, pos));
	_btnBox->Move(0, _btnUp->GetHeight() + ( _btnDown->GetY()
		- _btnBox->GetHeight() - _btnUp->GetHeight() ) * _pos / _limit);
}

float ScrollBar::GetPos() const
{
	return _pos;
}

void  ScrollBar::SetLimit(float limit)
{
	_limit = limit;
}

float ScrollBar::GetLimit() const
{
	return _limit;
}

void  ScrollBar::SetLineSize(float ls)
{
	_lineSize = ls;
}

void  ScrollBar::OnSize(float width, float height)
{
	_btnDown->Move(0, height - _btnDown->GetHeight());
    SetPos(_pos);  // update box position
}

void  ScrollBar::OnBoxMouseDown(float x, float y)
{
	_tmpBoxY = y;
}

void  ScrollBar::OnBoxMouseUp(float x, float y)
{
	_tmpBoxY = -1;
}

void  ScrollBar::OnBoxMouseMove(float x, float y)
{
	if( _tmpBoxY < 0 )
		return;
	float preferedY = _btnBox->GetY() + y - _tmpBoxY;
	float pos = (preferedY - _btnUp->GetHeight())
		/ (_btnDown->GetY() - _btnUp->GetHeight() - _btnBox->GetHeight());
	SetPos(pos * _limit);
	if( eventScroll )
		INVOKE(eventScroll) (GetPos());
}

void  ScrollBar::OnUp()
{
	SetPos(GetPos() - _lineSize);
}

void  ScrollBar::OnDown()
{
	SetPos(GetPos() + _lineSize);
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file