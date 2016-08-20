#pragma once
#include "Window.h"
#include "video/DrawingContext.h"

namespace UI
{
struct ColorSource;
struct TextSource;

class Text : public Window
{
public:
	Text(LayoutManager &manager, TextureManager &texman);

	void SetAlign(enumAlignText align);
	void SetFont(TextureManager &texman, const char *fontName);
	void SetFontColor(std::shared_ptr<ColorSource> color);

	void SetText(std::shared_ptr<TextSource> text);

	// Window
	void Draw(const StateContext &sc, const LayoutContext &lc, const InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;
	vec2d GetContentSize(const StateContext &sc, TextureManager &texman) const override;

private:
	enumAlignText  _align;
	size_t         _fontTexture;
	std::shared_ptr<ColorSource> _fontColor;
	std::shared_ptr<TextSource> _text;
};

} // namespace UI
