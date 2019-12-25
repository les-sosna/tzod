#include "inc/ui/Text.h"
#include "inc/ui/DataSource.h"
#include "inc/ui/LayoutContext.h"
#include <video/TextureManager.h>
#include <video/RenderContext.h>

using namespace UI;

void Text::SetFontColor(std::shared_ptr<RenderData<SpriteColor>> color)
{
	_fontColor = std::move(color);
}

void Text::SetText(std::shared_ptr<LayoutData<std::string_view>> text)
{
	_text = std::move(text);
}

void Text::Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, const Plat::Input &input, float time, bool hovered) const
{
	if (_text && !_fontTexture.Empty())
	{
		SpriteColor color = _fontColor ? _fontColor->GetRenderValue(dc, sc) : 0xffffffff;
		rc.DrawBitmapText(MakeRectWH(lc.GetPixelSize()), lc.GetScaleCombined(), _fontTexture.GetTextureId(texman), color, _text->GetLayoutValue(dc), _align);
	}
}

vec2d Text::GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const
{
	if (!_text)
		return vec2d{};

	unsigned int lineCount = 1;
	unsigned int maxline = 0;
	unsigned int count = 0;
	std::string_view text = _text->GetLayoutValue(dc);
	for( size_t n = 0; n != text.size(); ++n )
	{
		if( '\n' == text[n] )
		{
			if( maxline < count )
				maxline = count;
			++lineCount;
			count = 0;
		}
		++count;
	}
	if( 1 == lineCount )
	{
		maxline = static_cast<unsigned int>(text.size());
	}
	float w = std::floor(texman.GetFrameWidth(_fontTexture.GetTextureId(texman), 0) * scale);
	float h = std::floor(texman.GetFrameHeight(_fontTexture.GetTextureId(texman), 0) * scale);
	return vec2d{ (w - 1) * (float)maxline, h * (float)lineCount };
}

void TextWithUnderline::Draw(const DataContext& dc, const StateContext& sc, const LayoutContext& lc, const InputContext& ic, RenderContext& rc, TextureManager& texman, const Plat::Input &input, float time, bool hovered) const
{
	if (_text && !_fontTexture.Empty() &&
		_underline && _underline->GetRenderValue(dc, sc))
	{
		SpriteColor color = _fontColor ? _fontColor->GetRenderValue(dc, sc) : 0xffffffff;

		// grep enum enumAlignText LT CT RT LC CC RC LB CB RB
		static const float dx[] = { 0, 1, 2, 0, 1, 2, 0, 1, 2 };
		static const float dy[] = { 0, 0, 0, 1, 1, 1, 2, 2, 2 };

		vec2d contentSize = GetContentSize(texman, dc, lc.GetScaleCombined(), DefaultLayoutConstraints(lc));
		FRECT rect;
		rect.left = -std::floor(contentSize.x * dx[_align] / 2);
		rect.top = contentSize.y - std::floor(contentSize.y * dy[_align] / 2);
		rect.right = rect.left + contentSize.x;
		rect.bottom = rect.top + std::ceil(lc.GetScaleCombined());
		rc.DrawSprite(rect, _underlineTexture.GetTextureId(texman), color, 0);
	}
	Text::Draw(dc, sc, lc, ic, rc, texman, input, time, hovered);
}
