#pragma once
#include "Texture.h"
#include "Window.h"
#include "video/RenderContext.h"

namespace UI
{
template<class T> struct LayoutData;
template<class T> struct RenderData;

class Text : public Window
{
public:
	void SetAlign(enumAlignText align) { _align = align; }
	void SetFont(Texture fontTexture) { _fontTexture = std::move(fontTexture); }
	void SetFontColor(std::shared_ptr<RenderData<SpriteColor>> color);

	void SetText(std::shared_ptr<LayoutData<std::string_view>> text);

	// Window
	void Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const override;
	vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale) const override;

private:
	enumAlignText _align = alignTextLT;
	Texture _fontTexture = "font_small";
	std::shared_ptr<RenderData<SpriteColor>> _fontColor;
	std::shared_ptr<LayoutData<std::string_view>> _text;
};

} // namespace UI
