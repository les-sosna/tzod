// Scroll.cpp

#include "stdafx.h"
#include "Scroll.h"
#include "Button.h"

namespace UI
{
;
///////////////////////////////////////////////////////////////////////////////
// scrollbar class implementation

ScrollBar::ScrollBar(Window *parent, float x, float y, float size, bool hor)
  : Window(parent)
{
	_hor       = hor;
	_pos       = 0;
	_lineSize  = 1.0f;
	_limit     = 1.0f;
	_tmpBoxPos = -1;

	_btnUpLeft     = new ImageButton( this, 0, 0, hor ? "ctrl_scroll_left"  : "ctrl_scroll_up"   );
	_btnBox        = new ImageButton( this, 0, 0, hor ? "ctrl_scroll_hor"   : "ctrl_scroll_vert" );
	_btnDownRight  = new ImageButton( this, 0, 0, hor ? "ctrl_scroll_right" : "ctrl_scroll_down" );

	_btnUpLeft->eventClick.bind(&ScrollBar::OnUpLeft, this);
	_btnDownRight->eventClick.bind(&ScrollBar::OnDownRight, this);

	_btnBox->eventMouseDown.bind(&ScrollBar::OnBoxMouseDown, this);
	_btnBox->eventMouseUp.bind(  &ScrollBar::OnBoxMouseUp,   this);
	_btnBox->eventMouseMove.bind(&ScrollBar::OnBoxMouseMove, this);

	Resize(hor ? size : _btnUpLeft->GetWidth(), hor ? _btnUpLeft->GetHeight() : size);
	Move(x, y);
}

void ScrollBar::SetPos(float pos)
{
	_pos = __max(0, __min(_limit, pos));
	if( _hor )
	{
		_btnBox->Move(_btnUpLeft->GetWidth() + ( _btnDownRight->GetX()
			- _btnBox->GetWidth() - _btnUpLeft->GetWidth() ) * GetPos() / _limit, 0);
	}
	else
	{
		_btnBox->Move(0, _btnUpLeft->GetHeight() + ( _btnDownRight->GetY()
			- _btnBox->GetHeight() - _btnUpLeft->GetHeight() ) * GetPos() / _limit);
	}
}

float ScrollBar::GetPos() const
{
	return _pos;
}

void ScrollBar::SetLimit(float limit)
{
	_btnBox->Show(limit > 0);
	_btnUpLeft->Enable(limit > 0);
	_btnDownRight->Enable(limit > 0);

	_limit = limit;
	SetPos(GetPos()); // update scroll box position
}

float ScrollBar::GetLimit() const
{
	return _limit;
}

void ScrollBar::SetLineSize(float ls)
{
	_lineSize = ls;
}

void ScrollBar::OnSize(float width, float height)
{
	_btnDownRight->Move(  _hor ? width - _btnDownRight->GetWidth() : 0,
	                      _hor ? 0 : height - _btnDownRight->GetHeight() );
	SetPos(GetPos());  // to update scroll box position
}

void ScrollBar::OnBoxMouseDown(float x, float y)
{
	_tmpBoxPos = _hor ? x : y;
}

void ScrollBar::OnBoxMouseUp(float x, float y)
{
	_tmpBoxPos = -1;
}

void ScrollBar::OnBoxMouseMove(float x, float y)
{
	if( _tmpBoxPos < 0 )
		return;
	float pos;
	if( _hor )
	{
		float preferedX = _btnBox->GetX() + x - _tmpBoxPos;
		pos = (preferedX - _btnUpLeft->GetWidth())
			/ (_btnDownRight->GetX() - _btnUpLeft->GetWidth() - _btnBox->GetWidth());
	}
	else
	{
		float preferedY = _btnBox->GetY() + y - _tmpBoxPos;
		pos = (preferedY - _btnUpLeft->GetHeight())
			/ (_btnDownRight->GetY() - _btnUpLeft->GetHeight() - _btnBox->GetHeight());
	}
	SetPos(pos * _limit);
	if( eventScroll )
		INVOKE(eventScroll) (GetPos());
}

void ScrollBar::OnUpLeft()
{
	SetPos(GetPos() - _lineSize);
	if( eventScroll )
		INVOKE(eventScroll) (GetPos());
}

void ScrollBar::OnDownRight()
{
	SetPos(GetPos() + _lineSize);
	if( eventScroll )
		INVOKE(eventScroll) (GetPos());
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file