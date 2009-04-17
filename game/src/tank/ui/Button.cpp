// Button.cpp

#include "stdafx.h"

#include "Button.h"
#include "Text.h"


#include "core/Debug.h"
#include "core/Console.h"
#include "core/Application.h"


namespace UI
{

///////////////////////////////////////////////////////////////////////////////
// Button class implementation

ButtonBase::ButtonBase(Window *parent, float x, float y, const char *texture)
  : Window(parent, x,y, texture)
{
	_state = stateNormal;
}

void ButtonBase::SetState(State s)
{
	if( _state != s )
	{
		_state = s;
		OnChangeState(s);
	}
}

bool ButtonBase::OnMouseMove(float x, float y)
{
	if( IsCaptured() )
	{
		bool push = x < GetWidth() && y < GetHeight() && x > 0 && y > 0;
		SetState(push ? statePushed : stateNormal);
	}
	else
	{
		SetState(stateHottrack);
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
		SetState(statePushed);
		if( eventMouseDown )
			INVOKE(eventMouseDown) (x, y);
		return true;
	}
	return false;
}

bool ButtonBase::OnMouseUp(float x, float y, int button)
{
	if( IsCaptured() )
	{
		ReleaseCapture();
		bool click = (GetState() == statePushed);
		SafePtr<Window> holder(this); // prevent from immediate destroying
		if( eventMouseUp )
			INVOKE(eventMouseUp) (x, y);
		if( click && !IsDestroyed() )
			OnClick();
		if( !IsDestroyed() )
			SetState(stateHottrack);
		return true;
	}
	return false;
}

bool ButtonBase::OnMouseLeave()
{
	SetState(stateNormal);
	return true;
}

void ButtonBase::OnClick()
{
	if( eventClick )
		INVOKE(eventClick) ();
}

void ButtonBase::OnEnabledChange(bool enable, bool inherited)
{
	if( enable )
	{
		SetState(stateNormal);
	}
	else
	{
		SetState(stateDisabled);
	}
}


///////////////////////////////////////////////////////////////////////////////
// button class implementation

Button::Button(Window *parent, float x, float y, const string_t &text)
  : ButtonBase(parent, x, y, "ctrl_button")
{
	_label = new Text(this, 0, 0, text, alignTextCC);
	OnChangeState(stateNormal);
}

void Button::OnChangeState(State state)
{
	SetFrame(state);
	if( statePushed == state )
		_label->Move(GetWidth() / 2, GetHeight() / 2);
	else
		_label->Move(GetWidth() / 2 - 1, GetHeight() / 2 - 1);

	_label->SetFontColor(stateDisabled == state ? 0xbbbbbbbb : 0xffffffff);
}


///////////////////////////////////////////////////////////////////////////////
// text button class implementation

TextButton::TextButton(Window *parent, float x, float y, const string_t &text, const char *font)
  : ButtonBase(parent, x, y, NULL)
{
	_label = new Text(this, 0, 0, text, alignTextLT);
	_label->SetFont(font);
	Resize(_label->GetWidth(), _label->GetHeight());
	OnChangeState(stateNormal);
}

void TextButton::OnChangeState(State state)
{
	switch( state )
	{
	case stateNormal:
		_label->Move(0, 0);
		_label->SetFontColor(0xffffffff);
		break;
	case stateDisabled:
		_label->Move(0, 0);
		_label->SetFontColor(0xAAAAAAAA);
		break;
	case stateHottrack:
		_label->Move(0, 0);
		_label->SetFontColor(0xffccccff);
		break;
	case statePushed:
		_label->Move(1, 1);
		_label->SetFontColor(0xffccccff);
		break;
	}
}

void TextButton::SetText(const string_t &text)
{
	_label->SetText(text);
	Resize(_label->GetWidth(), _label->GetHeight());
}

const string_t& TextButton::GetText() const
{
	return _label->GetText();
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

CheckBox::CheckBox(Window *parent, float x, float y, const string_t &text)
  : ButtonBase(parent, x, y, "ctrl_checkbox")
  , _isChecked(false)
{
	_label = new TextButton(this, 0, 0, text, "font_small");
	_label->Move(GetWidth(), GetHeight() / 2 - _label->GetHeight() / 2);
	_label->eventClick.bind(&CheckBox::OnLabelClick, this);
}

void CheckBox::SetCheck(bool checked)
{
	_isChecked = checked;
	SetFrame(_isChecked ? GetState()+4 : GetState());
}

void CheckBox::SetText(const string_t &text)
{
	_label->SetText(text);
}

const string_t& CheckBox::GetText() const
{
	return _label->GetText();
}

void CheckBox::OnChangeState(State state)
{
	_label->SetEnabled(stateDisabled != state);
	SetFrame(_isChecked ? state+4 : state);
}

void CheckBox::OnClick()
{
	_isChecked = !_isChecked;
	SetFrame(_isChecked ? GetState()+4 : GetState());
	ButtonBase::OnClick();
}

void CheckBox::OnLabelClick()
{
	OnClick();
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

