#pragma once
#include <ui/Texture.h>
#include <ui/Window.h>
#include <string>
#include <deque>

namespace UI
{
	class ConsoleBuffer;
}

class MessageArea
	: public UI::Window
	, private UI::TimeStepping
{
public:
	MessageArea(UI::LayoutManager &manager, UI::ConsoleBuffer &logger);

	void WriteLine(std::string text);
	void Clear();

	// UI::Window
	void OnTimeStep(const UI::InputContext &ic, float dt) override;
	void Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const override;

private:
	struct Line
	{
		float time;
		std::string str;
	};
	typedef std::deque<Line> LineList;
	LineList _lines;
	UI::Texture _font = "font_small";
	UI::ConsoleBuffer &_logger;
};
