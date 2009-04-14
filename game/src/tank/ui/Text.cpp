// Text.cpp

#include "stdafx.h"
#include "Text.h"

#include "video/TextureManager.h"

namespace UI
{

///////////////////////////////////////////////////////////////////////////////
// Text class implementation

Text::Text(Window *parent, float x, float y, const string_t &text, enumAlignText align)
  : Window(parent, x, y, NULL)
  , _fontTexture(0)
  , _fontColor(0xffffffff)
  , _lineCount(1)
  , _maxline(0)
{
	SetAlign(align);
	SetText(text);
	SetFont("font_small");
}

void Text::SetText(const string_t &text)
{
	if( _text != text )
	{
		_text = text;

		// update lines
		_lineCount = 1;
		_maxline = 0;
		size_t count = 0;
		for( size_t n = 0; n != _text.size(); ++n )
		{
			if( '\n' == _text[n] )
			{
				if( _maxline < count )
					_maxline = count;
				++_lineCount;
				count = 0;
			}
			++count;
		}
		const LogicalTexture &lt = g_texman->Get(_fontTexture);
		Resize((lt.pxFrameWidth - 1) * (float) _maxline, lt.pxFrameHeight * (float) _lineCount);
	}
}

void Text::SetAlign(enumAlignText align)
{
	_align = align;
}

void Text::SetFont(const char *fontName)
{
	_fontTexture = _fontTexture = g_texman->FindTexture(fontName);
	_ASSERT(_fontTexture);
	const LogicalTexture &lt = g_texman->Get(_fontTexture);
	Resize(lt.pxFrameWidth * (float) _maxline, lt.pxFrameHeight * (float) _lineCount);
}

float Text::GetCharWidth()
{
	const LogicalTexture &lt = g_texman->Get(_fontTexture);
	return lt.pxFrameWidth - 1;
}

float Text::GetCharHeight()
{
	const LogicalTexture &lt = g_texman->Get(_fontTexture);
	return lt.pxFrameHeight;
}

void Text::DrawChildren(float sx, float sy) const
{
	g_texman->DrawBitmapText(_fontTexture, _text, _fontColor, sx, sy, _align);
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

