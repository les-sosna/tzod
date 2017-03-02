#pragma once
#include "Window.h"
#include "video/RenderContext.h"

namespace UI
{
template<class T> struct LayoutData;
template<class T> struct RenderData;

class Text : public Window
{
public:
	explicit Text(TextureManager &texman);

	void SetAlign(enumAlignText align);
	void SetFont(TextureManager &texman, const char *fontName);
	void SetFontColor(std::shared_ptr<RenderData<SpriteColor>> color);

	void SetText(std::shared_ptr<LayoutData<const std::string&>> text);

	// Window
	void Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const override;
	vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale) const override;

private:
	enumAlignText _align;
	size_t _fontTexture;
	std::shared_ptr<RenderData<SpriteColor>> _fontColor;
	std::shared_ptr<LayoutData<const std::string&>> _text;
};

} // namespace UI
