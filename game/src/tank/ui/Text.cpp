// Text.cpp

#include "stdafx.h"
#include "Text.h"

namespace UI
{

///////////////////////////////////////////////////////////////////////////////
// Text class implementation

Text::Text(Window *parent, float x, float y, const char *text, enumAlignText align)
: Window(parent, x, y, "font_small" /*"font_default"*/)
{
	SetAlign(align);
	SetText(text);

	_w = int(GetTextureWidth() - 1);
	_h = int(GetTextureHeight() - 1);
}

void Text::UpdateLines()
{
	_lines.clear();
	_maxline = 0;

	size_t count = 0;
	for( const char *tmp = _text.c_str(); *tmp; )
	{
		++count;
		++tmp;
		if( '\n' == *tmp || '\0' == *tmp )
		{
			if( count > _maxline ) _maxline = count;
			_lines.push_back(count);
			count = 0;
			continue;
		}
	}
}

void Text::SetText(const char *text)
{
	_ASSERT(NULL != text);
	if( _text != text )
	{
		_text = text;
		UpdateLines();
	}
}

void Text::SetAlign(enumAlignText align)
{
	_align = align;
}

float Text::GetTextWidth()
{
	return (float) _w * (float) _maxline;
}

float Text::GetTextHeight()
{
	return (float) _h * (float) _lines.size();
}

void Text::Draw(float sx, float sy)
{
	static const int dx_[] = {0, 1, 2, 0, 1, 2, 0, 1, 2};
	static const int dy_[] = {0, 0, 0, 1, 1, 1, 2, 2, 2};

	float x0 = sx - (float) (dx_[_align] * _w * _maxline / 2);
	float y0 = sy - (float) (dy_[_align] * _h * _lines.size() / 2);

	size_t count = 0;
	size_t line  = 0;

	for( const char *tmp = _text.c_str(); *tmp; ++tmp )
	{
		if( '\n' == *tmp )
		{
			++line;
			count = 0;
			continue;
		}
		SetFrame((unsigned char) *tmp - 32);
		Window::Draw( x0 + (float) ((count++) * _w), y0 + (float) (line * _h) );
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

