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
		_lines.clear();
		_maxline = 0;
		size_t count = 0;
		for( const TCHAR *tmp = _text.c_str(); *tmp; )
		{
			++count;
			++tmp;
			if( '\n' == *tmp || '\0' == *tmp )
			{
				if( count > _maxline )
				{
					_maxline = count;
				}
				_lines.push_back(count);
				count = 0;
				continue;
			}
		}
		if( _lines.empty() )
		{
			_lines.push_back(0);
		}
		const LogicalTexture &lt = g_texman->Get(_fontTexture);
		Resize((lt.pxFrameWidth - 1) * (float) _maxline, lt.pxFrameHeight * (float) _lines.size());
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
	Resize(lt.pxFrameWidth * (float) _maxline, lt.pxFrameHeight * (float) _lines.size());
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
	// grep enum enumAlignText LT CT RT LC CC RC LB CB RB
	static const float dx[] = { 0, 1, 2, 0, 1, 2, 0, 1, 2 };
	static const float dy[] = { 0, 0, 0, 1, 1, 1, 2, 2, 2 };

	const LogicalTexture &lt = g_texman->Get(_fontTexture);
	g_render->TexBind(lt.dev_texture);

	float x0 = sx - floorf(dx[_align] * (lt.pxFrameWidth - 1) * (float) _maxline / 2);
	float y0 = sy - floorf(dy[_align] * lt.pxFrameHeight * (float) _lines.size() / 2);


	g_texman->DrawBitmapText(_fontTexture, _text, _fontColor, x0, y0, _align);

/*
	size_t count = 0;
	size_t line  = 0;

	for( const TCHAR *tmp = _text.c_str(); *tmp; ++tmp )
	{
		if( '\n' == *tmp )
		{
			++line;
			count = 0;
			continue;
		}

		g_texman->DrawSprite(_fontTexture, (unsigned char) *tmp - 32, _fontColor,
			x0 + (float) ((count++) * (lt.pxFrameWidth - 1)), y0 + (float) (line * lt.pxFrameHeight), 0);
	}
*/
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

