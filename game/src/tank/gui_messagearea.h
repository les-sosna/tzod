#pragma once
#include <ui/Window.h>
#include <string>
#include <deque>

class MessageArea : public UI::Window
{
public:
	MessageArea(UI::Window *parent, float x, float y);

	void WriteLine(const std::string &text);
	void Clear();

	// UI::Window
	virtual void OnTimeStep(float dt);
	virtual void DrawChildren(DrawingContext &dc, float sx, float sy) const;

private:
	struct Line
	{
		float time;
		std::string str;
	};
	typedef std::deque<Line> LineList;
	LineList _lines;
	size_t _fontTexture;
};
