// Text.h

#pragma once

#include "Window.h"

namespace UI
{

// static text
class Text : public Window
{
	size_t           _lineCount;
	size_t           _maxline;
	enumAlignText    _align;
	string_t         _text;
	size_t           _fontTexture;
	SpriteColor      _fontColor;

public:
	Text(Window *parent, float x, float y, const string_t &text, enumAlignText align);

	void SetText(const string_t &text);
	const string_t& GetText() const { return _text; }
	void SetAlign(enumAlignText align);
	void SetFont(const char *fontName);
	void SetFontColor(SpriteColor color)       { _fontColor = color;   }

	float GetCharWidth();
	float GetCharHeight();

	virtual void DrawChildren(float sx, float sy) const;
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
