#include "MessageArea.h"
#include "inc/shell/Config.h"
#include <ui/ConsoleBuffer.h>
#include <ui/GuiManager.h>
#include <video/TextureManager.h>
#include <video/DrawingContext.h>
#include <algorithm>

MessageArea::MessageArea(Window *parent, ConfCache &conf, UI::ConsoleBuffer &logger)
  : Window(parent)
  , _fontTexture(GetManager().GetTextureManager().FindSprite("font_small"))
  , _conf(conf)
  , _logger(logger)
{
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

void MessageArea::DrawChildren(DrawingContext &dc) const
{
	if( _lines.empty() || !_conf.ui_showmsg.Get() )
	{
		return;
	}

	float h = GetManager().GetTextureManager().GetCharHeight(_fontTexture);
	float y = std::max(_lines.front().time - 4.5f, 0.0f) * h * 2;
	for( LineList::const_iterator it = _lines.begin(); it != _lines.end(); ++it )
	{
		unsigned char cc = std::min(int(it->time * 255 * 2), 255);
		SpriteColor c;
		c.r = cc;
		c.g = cc;
		c.b = cc;
		c.a = cc;

		dc.DrawBitmapText(0, y, _fontTexture, c, it->str);
		y -= h;
	}

	Window::DrawChildren(dc);
}

void MessageArea::WriteLine(const std::string &text)
{
	_logger.WriteLine(0, text);

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
