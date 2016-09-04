#pragma once
#include "Window.h"
#include "video/DrawingContext.h"

namespace UI
{
template<class T> struct DataSource;

class Text : public Window
{
public:
	Text(LayoutManager &manager, TextureManager &texman);

	void SetAlign(enumAlignText align);
	void SetFont(TextureManager &texman, const char *fontName);
	void SetFontColor(std::shared_ptr<DataSource<SpriteColor>> color);

	void SetText(std::shared_ptr<DataSource<const std::string&>> text);

	// Window
	void Draw(const StateContext &sc, const LayoutContext &lc, const InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;
	vec2d GetContentSize(TextureManager &texman, const StateContext &sc, float scale) const override;

private:
	enumAlignText  _align;
	size_t         _fontTexture;
	std::shared_ptr<DataSource<SpriteColor>> _fontColor;
	std::shared_ptr<DataSource<const std::string&>> _text;
};

} // namespace UI
