// gui_desktop.h

#pragma once

#include "Window.h"
#include "Console.h"
#include "Level.h"// for EditorModeListenerHelper

namespace UI
{
	class EditorLayout;
	class Console;
	class ScoreTable;

	// widgets
	class FpsCounter;
	class TimeElapsed;
	class Oscilloscope;

///////////////////////////////////////////////////////////////////////////////

class MessageArea : public Window
{
private:
	struct Line
	{
		float time;
		string_t str;
	};
	typedef std::deque<Line> LineList;
	LineList _lines;
	size_t _fontTexture;

public:
	MessageArea(Window *parent, float x, float y);
	~MessageArea();

	void WriteLine(const string_t &text);
	void Clear();

	virtual void OnTimeStep(float dt);
	virtual void DrawChildren(const DrawingContext *dc, float sx, float sy) const;

private:
	void OnToggleVisible();
};

///////////////////////////////////////////////////////////////////////////////

class Desktop
	: public Window
	, private EditorModeListenerHelper
{
	class MyConsoleHistory : public UI::IConsoleHistory
	{
	public:
		virtual void Enter(const UI::string_t &str);
		virtual size_t GetItemCount() const;
		virtual const UI::string_t& GetItem(size_t index) const;
	};

	MyConsoleHistory  _history;

	EditorLayout *_editor;
	Console      *_con;
	MessageArea  *_msg;
	ScoreTable   *_score;

	FpsCounter   *_fps;
	TimeElapsed  *_time;

	// IEditorModeListener
	virtual void OnEditorModeChanged(bool editorMode);

public:
	Desktop(LayoutManager* manager);
	virtual ~Desktop();

	virtual void DrawChildren(const DrawingContext *dc, float sx, float sy) const;

	void ShowConsole(bool show);

	void OnCloseChild(int result);

	MessageArea* GetMsgArea() const;

protected:
	virtual bool OnRawChar(int c);
	virtual bool OnFocus(bool focus);
	virtual void OnSize(float width, float height);

private:
	void OnChangeShowFps();
	void OnChangeShowTime();

	void OnCommand(const string_t &cmd);
	bool OnCompleteCommand(const string_t &cmd, int &pos, string_t &result);
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
