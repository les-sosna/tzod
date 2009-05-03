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
public:
	enum State
	{
		stateNormal    = 0,
		stateHottrack  = 1,
		statePushed    = 2,
		stateDisabled  = 3
	};

	ButtonBase(Window *parent, float x, float y, const char *texture);

	Delegate<void(void)> eventClick;
	Delegate<void(float, float)> eventMouseDown;
	Delegate<void(float, float)> eventMouseUp;
	Delegate<void(float, float)> eventMouseMove;

	State GetState() const { return _state; }
	void SetState(State s);

protected:
	virtual bool OnMouseMove(float x, float y);
	virtual bool OnMouseDown(float x, float y, int nButton);
	virtual bool OnMouseUp  (float x, float y, int nButton);
	virtual bool OnMouseLeave();

	virtual void OnEnabledChange(bool enable, bool inherited);
	virtual void OnClick();
	virtual void OnChangeState(State state) = 0;

private:
	State _state;
};

///////////////////////////////////////////////////////////////////////////////

class Button : public ButtonBase
{
	Text  *_label;

public:
	Button(Window *parent, const string_t &text, float x, float y, float w=-1, float h=-1);

protected:
	virtual void OnChangeState(State state);
};

///////////////////////////////////////////////////////////////////////////////

class TextButton : public ButtonBase
{
	Text  *_label;

public:
	TextButton(Window *parent, float x, float y, const string_t &text, const char *font);

	void SetText(const string_t &text);
	const string_t& GetText() const;

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
	TextButton *_label;
	bool  _isChecked;

public:
	CheckBox(Window *parent, float x, float y, const string_t &text);

	void SetCheck(bool checked);
	bool GetCheck() const { return _isChecked; }

	void SetText(const string_t &text);
	const string_t& GetText() const;

protected:
	virtual void OnChangeState(State state);
	virtual void OnClick();

private:
	void OnLabelClick();
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file