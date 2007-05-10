// Text.h

#pragma once

#include "Window.h"

namespace UI
{

///////////////////////////////////////////////////////////////////////////////

class Text : public Window
{
	std::vector<size_t>  _lines;    // длины строк
	size_t               _maxline;  // макс. длина строки
	enumAlignText        _align;
	string_t          _text;

	int _w, _h;

private:
	void UpdateLines();

public:
	Text(Window *parent, float x, float y, const char *text, enumAlignText align);

	void SetText(const char *lpszText);
	void SetAlign(enumAlignText align);

	virtual void Draw(float sx = 0, float sy = 0);
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
