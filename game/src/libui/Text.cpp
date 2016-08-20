#include "inc/ui/Text.h"
#include "inc/ui/DataSource.h"
#include "inc/ui/GuiManager.h"
#include <video/TextureManager.h>
#include <video/DrawingContext.h>

using namespace UI;

Text::Text(LayoutManager &manager, TextureManager &texman)
  : Window(manager)
  , _align(alignTextLT)
  , _fontTexture(0)
{
	SetFont(texman, "font_small");
}

void Text::SetAlign(enumAlignText align)
{
	_align = align;
}

void Text::SetFont(TextureManager &texman, const char *fontName)
{
	_fontTexture = texman.FindSprite(fontName);
}

void Text::SetFontColor(std::shared_ptr<ColorSource> color)
{
	_fontColor = std::move(color);
}

void Text::SetText(std::shared_ptr<TextSource> text)
{
	_text = std::move(text);
}

void Text::Draw(const StateContext &sc, const LayoutContext &lc, const InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	if (_text)
	{
		dc.DrawBitmapText(0, 0, _fontTexture, _fontColor ? _fontColor->GetColor(sc) : 0xffffffff, _text->GetText(sc), _align);
	}
}

vec2d Text::GetContentSize(const StateContext &sc, TextureManager &texman) const
{
	// update lines
	unsigned int lineCount = 1;
	unsigned  maxline = 0;
	size_t count = 0;
	const std::string &text = _text->GetText(sc);
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
	float w = texman.GetFrameWidth(_fontTexture, 0);
	float h = texman.GetFrameHeight(_fontTexture, 0);
	return vec2d{ (w - 1) * (float)maxline, h * (float)lineCount };
}

