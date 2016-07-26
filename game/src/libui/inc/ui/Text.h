#pragma once
#include "Window.h"
#include "video/DrawingContext.h"

namespace UI
{
struct ColorSource;

class Text : public Window
{
public:
	Text(LayoutManager &manager, TextureManager &texman);

	void SetAlign(enumAlignText align);
	void SetFont(TextureManager &texman, const char *fontName);
	void SetFontColor(std::shared_ptr<ColorSource> color);

	void Draw(const StateContext &sc, const LayoutContext &lc, const InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;
	void OnTextChange(TextureManager &texman) override;

private:
	size_t         _lineCount;
	size_t         _maxline;
	enumAlignText  _align;
	size_t         _fontTexture;
	std::shared_ptr<ColorSource> _fontColor;
};

} // namespace UI
