#include "MessageArea.h"
#include <plat/ConsoleBuffer.h>
#include <ui/GuiManager.h>
#include <ui/LayoutContext.h>
#include <video/TextureManager.h>
#include <video/RenderContext.h>
#include <algorithm>

MessageArea::MessageArea(UI::TimeStepManager &manager, Plat::ConsoleBuffer &logger)
  : UI::TimeStepping(manager)
  , _logger(logger)
{
}

void MessageArea::OnTimeStep(const Plat::Input &input, bool focused, float dt)
{
	for( size_t i = 0; i < _lines.size(); ++i )
		_lines[i].time -= dt;
	while( !_lines.empty() && _lines.back().time <= 0 )
		_lines.pop_back();

	if( _lines.empty() )
	{
		SetTimeStep(false);
	}
}

void MessageArea::Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, float time, bool hovered) const
{
	if( _lines.empty() )
	{
		return;
	}

	float h = texman.GetCharHeight(_font.GetTextureId(texman));
	float y = std::max(_lines.front().time - 4.5f, 0.0f) * h * 2;
	for( LineList::const_iterator it = _lines.begin(); it != _lines.end(); ++it )
	{
		unsigned char cc = std::min(int(it->time * 255 * 2), 255);
		SpriteColor c;
		c.r = cc;
		c.g = cc;
		c.b = cc;
		c.a = cc;

		rc.DrawBitmapText(vec2d{ 0, y }, lc.GetScaleCombined(), _font.GetTextureId(texman), c, it->str);
		y -= h;
	}
}

void MessageArea::WriteLine(std::string text)
{
	_logger.WriteLine(0, text);

	Line line;
	line.time = 5;  // timeout
	line.str = std::move(text);
	_lines.push_front(std::move(line));

	SetTimeStep(true);
}

void MessageArea::Clear()
{
	_lines.clear();
	SetTimeStep(false);
}
