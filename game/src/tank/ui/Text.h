// Text.h

#pragma once

#include "Window.h"

namespace UI
{

// static text
class Text : public Window
{
	std::vector<size_t>  _lines;    // длины строк
	size_t               _maxline;  // макс. длина строки
	enumAlignText        _align;
	string_t             _text;

	int _w, _h;

private:
	void UpdateLines();
	void OnSize(float width, float height);

public:
	Text(Window *parent, float x, float y, const string_t &text, enumAlignText align);

	void SetText(const string_t &text);
	const string_t& GetText() const { return _text; }
	void SetAlign(enumAlignText align);

	float GetTextWidth();
	float GetTextHeight();

	virtual void Draw(float sx = 0, float sy = 0);
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
