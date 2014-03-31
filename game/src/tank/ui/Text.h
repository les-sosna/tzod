// Text.h

#pragma once

#include "Window.h"

namespace UI
{

// static text
class Text : public Window
{
public:
	static Text* Create(Window *parent, float x, float y, const std::string &text, enumAlignText align);

	void SetDrawShadow(bool drawShadow);
	bool GetDrawShadow() const;

	void SetAlign(enumAlignText align);
	void SetFont(const char *fontName);
	void SetFontColor(SpriteColor color);

	float GetCharWidth();
	float GetCharHeight();

	virtual void DrawChildren(const DrawingContext *dc, float sx, float sy) const;
	virtual void OnTextChange();

protected:
	Text(Window *parent);

private:
	size_t         _lineCount;
	size_t         _maxline;
	enumAlignText  _align;
	size_t         _fontTexture;
	SpriteColor    _fontColor;

	bool           _drawShadow;
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
