// gui_desktop.h

#pragma once

#include "Window.h"

namespace UI
{
	class EditorLayout;
	class Console;
	class ScoreTable;

	// widgets
	class FpsCounter;
	class TimeElapsed;

///////////////////////////////////////////////////////////////////////////////

class MessageArea : public Window
{
private:
	struct Line
	{
		float time;
		string_t str;
	};
	std::deque<Line> _lines;
	Text *_text;

public:
	MessageArea(Window *parent, float x, float y);
	~MessageArea();

	void puts(const char *text);
	void Clear();

	void OnTimeStep(float dt);

private:
	void OnToggleVisible();
};

///////////////////////////////////////////////////////////////////////////////

class Desktop : public Window
{
	EditorLayout *_editor;
	Console      *_con;
	MessageArea  *_msg;
	ScoreTable   *_score;

	FpsCounter   *_fps;
	TimeElapsed  *_time;

public:
	Desktop(GuiManager* manager);
	~Desktop();

	void ShowDesktopBackground(bool show);
	void ShowConsole(bool show);
	void ShowEditor(bool show);

	void OnCloseChild(int result);

	MessageArea* GetMsgArea() const;

protected:
	virtual void OnRawChar(int c);
	virtual bool OnFocus(bool focus);
	virtual void OnSize(float width, float height);

private:
	void OnChangeShowFps();
	void OnChangeShowTime();

	void OnCommand(const char *cmd);
	bool OnCompleteCommand(const char *cmd, string_t &result);
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
