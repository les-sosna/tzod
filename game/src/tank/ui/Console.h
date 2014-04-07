// Console.h

#pragma once

#include "Base.h"
#include "Window.h"

#include <deque>
#include <string>
#include <functional>
#include <vector>

namespace UI
{

class ConsoleBuffer;

///////////////////////////////////////////////////////////////////////////////

struct IConsoleHistory
{
	virtual void Enter(const std::string &str) = 0;
	virtual size_t GetItemCount() const = 0;
	virtual const std::string& GetItem(size_t index) const = 0;
};

///////////////////////////////////////////////////////////////////////////////

class ConsoleHistoryDefault : public IConsoleHistory
{
public:
	ConsoleHistoryDefault(size_t maxSize);

	virtual void Enter(const std::string &str);
	virtual size_t GetItemCount() const;
	virtual const std::string& GetItem(size_t index) const;

private:
	size_t _maxSize;
	std::deque<std::string> _buf;
};

///////////////////////////////////////////////////////////////////////////////

class Console : public Window
{
public:
	Console(Window *parent);
	static Console* Create(Window *parent, float x, float y, float w, float h, ConsoleBuffer *buf);

	float GetInputHeight() const;

	void SetColors(const SpriteColor *colors, size_t count);
	void SetHistory(IConsoleHistory *history);
	void SetBuffer(ConsoleBuffer *buf);
	void SetEcho(bool echo);
	std::function<void(const std::string &)> eventOnSendCommand;
	std::function<bool(const std::string &, int &, std::string &)> eventOnRequestCompleteCommand;

protected:
	virtual bool OnChar(int c);
	virtual bool OnRawChar(int c);
	virtual bool OnMouseWheel(float x, float y, float z);
	virtual bool OnMouseDown(float x, float y, int button);
	virtual bool OnMouseUp(float x, float y, int button);
	virtual bool OnMouseMove(float x, float y);

	virtual void OnTimeStep(float dt);
	virtual void DrawChildren(const DrawingContext *dc, float sx, float sy) const;
	virtual void OnSize(float width, float height);
	virtual bool OnFocus(bool focus);

private:
	void OnScroll(float pos);

private:
	ScrollBarVertical *_scroll;
	Edit  *_input;
	size_t _cmdIndex;
	size_t _font;

	ConsoleBuffer *_buf;
	IConsoleHistory *_history;
	std::vector<SpriteColor> _colors;

	bool _echo;
	bool _autoScroll;
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
