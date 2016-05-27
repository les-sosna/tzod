#include "MessageArea.h"
#include "inc/shell/Config.h"
#include <ui/ConsoleBuffer.h>
#include <ui/GuiManager.h>
#include <video/TextureManager.h>
#include <video/DrawingContext.h>
#include <algorithm>

MessageArea::MessageArea(UI::LayoutManager &manager, TextureManager &texman, ConfCache &conf, UI::ConsoleBuffer &logger)
  : Window(manager)
  , _fontTexture(texman.FindSprite("font_small"))
  , _conf(conf)
  , _logger(logger)
{
}

void MessageArea::OnTimeStep(UI::LayoutManager &manager, float dt)
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

void MessageArea::Draw(bool hovered, bool focused, bool enabled, vec2d size, UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	if( _lines.empty() || !_conf.ui_showmsg.Get() )
	{
		return;
	}

	Window::Draw(hovered, focused, enabled, size, ic, dc, texman);

	float h = texman.GetCharHeight(_fontTexture);
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
