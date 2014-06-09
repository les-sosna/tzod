#pragma once

#include <Window.h>
#include <string>
#include <deque>


namespace UI
{

class MessageArea : public Window
{
public:
	MessageArea(Window *parent, float x, float y);

	void WriteLine(const std::string &text);
	void Clear();

    // Window
	virtual void OnTimeStep(float dt);
	virtual void DrawChildren(DrawingContext &dc, float sx, float sy) const;

private:
	void OnToggleVisible();

	struct Line
	{
		float time;
		std::string str;
	};
	typedef std::deque<Line> LineList;
	LineList _lines;
	size_t _fontTexture;
};


} // end of namespace UI
