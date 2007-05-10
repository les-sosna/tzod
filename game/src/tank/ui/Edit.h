// Edit.h

#pragma once

#include "Window.h"


namespace UI
{
// forward declarations
class Text;

///////////////////////////////////////////////////////////////////////////////
// simple EditBox

class Edit : public Window
{
	Text   *_blankText;
	Window *_cursor;

	int   _selStart;
	int   _selEnd;

	string_t _string;

public:
	Edit(Window *parent, float x, float y, float width);

	const string_t& GetText() const;
    void SetText(const char *text);

    void SetInt(int value);
	int  GetInt() const;

	void SetSel(int begin, int end);

protected:
	virtual void OnChar(int c);
	virtual void OnRawChar(int c);
	virtual bool OnMouseDown(float x, float y, int button);
	virtual void Draw(float sx, float sy);
	virtual bool OnFocus(bool focus);
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
