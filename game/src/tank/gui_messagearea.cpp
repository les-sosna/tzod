#include "gui_messagearea.h"

#include "config/Config.h"
#include "video/TextureManager.h"

#include <GuiManager.h>
#include <ConsoleBuffer.h>

#include <algorithm>

UI::ConsoleBuffer& GetConsole();


namespace UI
{

MessageArea::MessageArea(Window *parent, float x, float y)
  : Window(parent)
  , _fontTexture(GetManager()->GetTextureManager().FindSprite("font_small"))
{
	Move(x, y);
}

void MessageArea::OnTimeStep(float dt)
{
	for( size_t i = 0; i < _lines.size(); ++i )
		_lines[i].time -= dt;
	while( !_lines.empty() && _lines.back().time <= 0 )
		_lines.pop_back();

	if( _lines.empty() )
	{
		SetTimeStep(false);
		return;
	}
}

void MessageArea::DrawChildren(DrawingContext &dc, float sx, float sy) const
{
	if( _lines.empty() || !g_conf.ui_showmsg.Get() )
	{
		return;
	}

	float h = GetManager()->GetTextureManager().GetCharHeight(_fontTexture);
	float y = std::max(_lines.front().time - 4.5f, 0.0f) * h * 2;
	for( LineList::const_iterator it = _lines.begin(); it != _lines.end(); ++it )
	{
		unsigned char cc = std::min(int(it->time * 255 * 2), 255);
		SpriteColor c;
		c.r = cc;
		c.g = cc;
		c.b = cc;
		c.a = cc;

		dc.DrawBitmapText(sx, sy + y, _fontTexture, c, it->str);
		y -= h;
	}
}

void MessageArea::WriteLine(const std::string &text)
{
	GetConsole().WriteLine(0, text);

	Line line;
	line.time = 5;  // timeout
	line.str = text;
	_lines.push_front(line);

	SetTimeStep(true);
}

void MessageArea::Clear()
{
	_lines.clear();
	SetTimeStep(false);
}


} // end of namespace UI
