// Button.cpp

#include "stdafx.h"

#include "Button.h"
#include "Text.h"

namespace UI
{

///////////////////////////////////////////////////////////////////////////////
// Button class implementation

ButtonBase::ButtonBase(Window *parent, float x, float y, const char *texture)
: Window(parent, x,y, texture)
{
	_isPressed = false;
}

void ButtonBase::_state(bool pressed)
{
	if( _isPressed = pressed )
	{
		OnChangeState(statePushed); // pressed
	}
	else
	{
		OnChangeState(stateNormal); // normal
	}
}

bool ButtonBase::OnMouseMove(float x, float y)
{
	if( IsCaptured() )
	{
		_state(x < GetWidth() && y < GetHeight() && x > 0 && y > 0);
	}
	else
	{
		OnChangeState(stateHottrack);
	}
	if( eventMouseMove )
   		INVOKE(eventMouseMove) (x, y);
	return true;
}

bool ButtonBase::OnMouseDown(float x, float y, int button)
{
	if( 1 == button ) // left button only
	{
		SetCapture();
		_state(true);
		if( eventMouseDown )
			INVOKE(eventMouseDown) (x, y);
	}
	return true;
}

bool ButtonBase::OnMouseUp(float x, float y, int button)
{
	if( IsCaptured() )
	{
		ReleaseCapture();
		bool click = _isPressed;
		_state(false);
		if( eventMouseUp )
			INVOKE(eventMouseUp) (x, y);
		if( click )
			OnClick();
	}
	return true;
}

bool ButtonBase::OnMouseLeave()
{
	OnChangeState(stateNormal);
	return true;
}

void ButtonBase::OnClick()
{
	if( eventClick )
		INVOKE(eventClick) ();
}

void ButtonBase::OnEnable(bool enable)
{
	if( enable )
	{
		OnChangeState(_isPressed ? statePushed : stateNormal);
	}
	else
	{
		OnChangeState(stateDisabled);
	}
}


///////////////////////////////////////////////////////////////////////////////
// button class implementation

Button::Button(Window *parent, float x, float y, const char *text)
  : ButtonBase(parent, x, y, "ctrl_button")
{
	_label = new Text(this, 0, 0, text, alignTextCC );
	OnChangeState(stateNormal); // to update label position
}

void Button::OnChangeState(State state)
{
	SetFrame(state);
	if( statePushed == state )
		_label->Move(GetTextureWidth()/2, GetTextureHeight()/2);
	else
		_label->Move(GetTextureWidth()/2-1, GetTextureHeight()/2-1);

	_label->SetColor(stateDisabled == state ? 0xffbbbbbb : 0xffffffff);
}

///////////////////////////////////////////////////////////////////////////////
// ImageButton class implementation

ImageButton::ImageButton(Window *parent, float x, float y, const char *texture)
  : ButtonBase(parent, x, y, texture)
{
}

void ImageButton::OnChangeState(State state)
{
	SetFrame(state);
}

///////////////////////////////////////////////////////////////////////////////
// CheckBox class implementation

CheckBox::CheckBox(Window *parent, float x, float y, const char *text)
: ButtonBase(parent, x, y, "ctrl_checkbox")
{
	_label = new Text(this, GetTextureWidth(), GetTextureHeight()/2, text, alignTextLC );
	_isChecked = false;
}

void CheckBox::OnChangeState(State state)
{
	SetFrame(_isChecked ? state+4 : state);
}

void CheckBox::OnClick()
{
	_isChecked = !_isChecked;
	ButtonBase::OnClick();
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

