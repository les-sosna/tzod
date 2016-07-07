#pragma once
#include "Window.h"
#include "video/DrawingContext.h"

namespace UI
{

class Text : public Window
{
public:
	Text(LayoutManager &manager, TextureManager &texman);

	void SetAlign(enumAlignText align);
	void SetFont(TextureManager &texman, const char *fontName);
	void SetFontColor(SpriteColor color);

	void Draw(const LayoutContext &lc, InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;
	void OnTextChange(TextureManager &texman) override;

private:
	size_t         _lineCount;
	size_t         _maxline;
	enumAlignText  _align;
	size_t         _fontTexture;
	SpriteColor    _fontColor;
};

} // namespace UI
