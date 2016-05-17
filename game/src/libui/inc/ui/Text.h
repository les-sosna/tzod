// Text.h

#pragma once

#include "Window.h"
#include "video/DrawingContext.h"

namespace UI
{

// static text
class Text : public Window
{
public:
	static std::shared_ptr<Text> Create(Window *parent, TextureManager &texman, float x, float y, const std::string &text, enumAlignText align);

	Text(LayoutManager &manager, TextureManager &texman);

	void SetDrawShadow(bool drawShadow);
	bool GetDrawShadow() const;

	void SetAlign(enumAlignText align);
	void SetFont(TextureManager &texman, const char *fontName);
	void SetFontColor(SpriteColor color);

	float GetCharWidth();
	float GetCharHeight();

	void Draw(DrawingContext &dc, TextureManager &texman) const override;
	void OnTextChange() override;

private:
	size_t         _lineCount;
	size_t         _maxline;
	enumAlignText  _align;
	size_t         _fontTexture;
	SpriteColor    _fontColor;

	bool           _drawShadow;
};

} // namespace UI
