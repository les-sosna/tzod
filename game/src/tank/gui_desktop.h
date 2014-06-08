// gui_desktop.h

#pragma once

#include "InputManager.h"
#include "WorldEvents.h"
#include <Window.h>
#include <Console.h>

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
		std::string str;
	};
	typedef std::deque<Line> LineList;
	LineList _lines;
	size_t _fontTexture;

public:
	MessageArea(Window *parent, float x, float y);
	~MessageArea();

	void WriteLine(const std::string &text);
	void Clear();

	virtual void OnTimeStep(float dt);
	virtual void DrawChildren(const DrawingContext *dc, float sx, float sy) const;

private:
	void OnToggleVisible();
};

///////////////////////////////////////////////////////////////////////////////

class Desktop
	: public Window
    , private MessageListener
{
	class MyConsoleHistory : public UI::IConsoleHistory
	{
	public:
		virtual void Enter(const std::string &str);
		virtual size_t GetItemCount() const;
		virtual const std::string& GetItem(size_t index) const;
	};

	MyConsoleHistory  _history;
    InputManager _inputMgr;

	EditorLayout *_editor;
	Console      *_con;
	MessageArea  *_msg;
	ScoreTable   *_score;

	FpsCounter   *_fps;
	TimeElapsed  *_time;

	size_t _font;
    
    int _nModalPopups;
    
    World &_world;

    void SetEditorMode(bool editorMode);
    bool IsGamePaused() const;

public:
	Desktop(LayoutManager* manager, World &_world);
	virtual ~Desktop();

    virtual void OnTimeStep(float dt);
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

	void OnCommand(const std::string &cmd);
	bool OnCompleteCommand(const std::string &cmd, int &pos, std::string &result);
    
    // MessageListener
    virtual void OnGameMessage(const char *msg);
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
