#include "inc/ui/Text.h"
#include "inc/ui/DataSource.h"
#include "inc/ui/GuiManager.h"
#include "inc/ui/LayoutContext.h"
#include <video/TextureManager.h>
#include <video/RenderContext.h>

using namespace UI;

void Text::SetFontColor(std::shared_ptr<RenderData<SpriteColor>> color)
{
	_fontColor = std::move(color);
}

void Text::SetText(std::shared_ptr<LayoutData<const std::string&>> text)
{
	_text = std::move(text);
}

void Text::Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const
{
	if (_text && !_fontTexture.Empty())
	{
		rc.DrawBitmapText(vec2d{}, lc.GetScale(), _fontTexture.GetTextureId(texman), _fontColor ? _fontColor->GetValue(dc, sc) : 0xffffffff, _text->GetValue(dc), _align);
	}
}

vec2d Text::GetContentSize(TextureManager &texman, const DataContext &dc, float scale) const
{
	if (!_text)
		return vec2d{};

	unsigned int lineCount = 1;
	unsigned  maxline = 0;
	size_t count = 0;
	const std::string &text = _text->GetValue(dc);
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
		maxline = text.size();
	}
	float w = std::floor(texman.GetFrameWidth(_fontTexture.GetTextureId(texman), 0) * scale);
	float h = std::floor(texman.GetFrameHeight(_fontTexture.GetTextureId(texman), 0) * scale);
	return vec2d{ (w - 1) * (float)maxline, h * (float)lineCount };
}

