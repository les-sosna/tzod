// Button.h

#pragma once

#include "Base.h"
#include "Window.h"

///////////////////////////////////////////////////////////////////////////////

namespace UI
{

// base class for all button stuff
class ButtonBase : public Window
{
	bool _isPressed;
	void _state(bool pressed);

protected:
	enum State
	{
		stateNormal    = 0,
		stateHottrack  = 1,
		statePushed    = 2,
		stateDisabled  = 3
	};

public:
	ButtonBase(Window *parent, float x, float y, const char *texture);

	Delegate<void(void)> eventClick;
	Delegate<void(float, float)> eventMouseDown;
	Delegate<void(float, float)> eventMouseUp;
	Delegate<void(float, float)> eventMouseMove;

protected:
	virtual bool OnMouseMove(float x, float y);
	virtual bool OnMouseDown(float x, float y, int nButton);
	virtual bool OnMouseUp  (float x, float y, int nButton);
	virtual bool OnMouseLeave();

protected:
	virtual void OnEnable(bool enable);
	virtual void OnClick();
	virtual void OnChangeState(State state) = 0;
};

///////////////////////////////////////////////////////////////////////////////

class Button : public ButtonBase
{
	Text  *_label;

public:
	Button(Window *parent, float x, float y, const char *text);

protected:
	virtual void OnChangeState(State state);
};

///////////////////////////////////////////////////////////////////////////////

class ImageButton : public ButtonBase
{
public:
	ImageButton(Window *parent, float x, float y, const char *texture);

protected:
	virtual void OnChangeState(State state);
};

///////////////////////////////////////////////////////////////////////////////

class CheckBox : public ButtonBase
{
	Text *_label;
	bool  _isChecked;

public:
	CheckBox(Window *parent, float x, float y, const char *text);

	void SetCheck(bool checked) { _isChecked = checked; }
	bool GetCheck() const { return _isChecked; }

protected:
	virtual void OnChangeState(State state);
	virtual void OnClick();
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file