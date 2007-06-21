// gui_console.h

#pragma once

#include "ui/Base.h"
#include "ui/Window.h"


namespace UI
{
///////////////////////////////////////////////////////////////////////////////

class Console : public Window
{
	Text  *_blankText;
	Text  *_arrow;
	Edit  *_input;
	size_t _scrollBack;
	size_t _cmdIndex;

public:
	Console(Window *parent, float x, float y);

protected:
	virtual void OnChar(int c);
	virtual void OnRawChar(int c);
	virtual bool OnMouseWheel(float x, float y, float z);

	virtual void DrawChildren(float sx, float sy);
	virtual void OnShow(bool show);
	virtual void OnSize(float width, float height);
	virtual bool OnFocus(bool focus);
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
