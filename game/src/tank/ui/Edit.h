// Edit.h

#pragma once

#include "Base.h"
#include "Window.h"


namespace UI
{

///////////////////////////////////////////////////////////////////////////////
// simple EditBox

class Edit : public Window
{
	Text   *_blankText;
	Window *_cursor;
	Window *_selection;

	int   _selStart;
	int   _selEnd;

	string_t _string;

	float _time;

public:
	Edit(Window *parent, float x, float y, float width);

	const string_t& GetText() const;
	void SetText(const string_t &text);
	int GetTextLength() const;

	void SetInt(int value);
	int  GetInt() const;

	void  SetFloat(float value);
	float GetFloat() const;

	void SetSel(int begin, int end); // -1 means end of string
	int GetSelStart() const;
	int GetSelEnd() const;
	int GetSelMin() const;
	int GetSelMax() const;
	int GetSelLength() const;

	void Paste();
	void Copy() const;

	Delegate<void()> eventChange;

protected:
	virtual void OnChar(int c);
	virtual void OnRawChar(int c);
	virtual bool OnMouseDown(float x, float y, int button);
	virtual bool OnMouseUp(float x, float y, int button);
	virtual bool OnMouseMove(float x, float y);
	virtual bool OnFocus(bool focus);
	virtual void OnEnabledChange(bool enable);
	virtual void OnTimeStep(float dt);
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
