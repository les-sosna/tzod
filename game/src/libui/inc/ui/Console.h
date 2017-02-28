#pragma once
#include "Rectangle.h"
#include <deque>
#include <functional>
#include <string>
#include <vector>

class TextureManager;

namespace UI
{

class ConsoleBuffer;
class ScrollBarVertical;
class Edit;

struct IConsoleHistory
{
	virtual void Enter(std::string str) = 0;
	virtual size_t GetItemCount() const = 0;
	virtual const std::string& GetItem(size_t index) const = 0;
};

class ConsoleHistoryDefault : public IConsoleHistory
{
public:
	ConsoleHistoryDefault(size_t maxSize);

	void Enter(std::string str) override;
	size_t GetItemCount() const override;
	const std::string& GetItem(size_t index) const override;

private:
	size_t _maxSize;
	std::deque<std::string> _buf;
};

///////////////////////////////////////////////////////////////////////////////

class Console
	: public Rectangle
	, private Managerful
	, private ScrollSink
	, private KeyboardSink
{
public:
	Console(LayoutManager &manager, TextureManager &texman);
	static std::shared_ptr<Console> Create(Window *parent, LayoutManager &manager, TextureManager &texman, float x, float y, float w, float h, ConsoleBuffer *buf);

	void SetColors(const SpriteColor *colors, size_t count);
	void SetHistory(IConsoleHistory *history);
	void SetBuffer(ConsoleBuffer *buf);
	void SetEcho(bool echo);
	std::function<void(const std::string &)> eventOnSendCommand;
	std::function<bool(const std::string &, int &, std::string &)> eventOnRequestCompleteCommand;

	// Window
	ScrollSink* GetScrollSink() override { return this; }
	KeyboardSink *GetKeyboardSink() override { return this; }
	void Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman) const override;
	FRECT GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const override;

	// Managerful
	void OnTimeStep(LayoutManager &manager, float dt) override;

private:
	std::shared_ptr<ScrollBarVertical> _scroll;
	std::shared_ptr<Edit> _input;
	size_t _cmdIndex;
	size_t _font;

	ConsoleBuffer *_buf;
	IConsoleHistory *_history;
	std::vector<SpriteColor> _colors;

	bool _echo;
	bool _autoScroll;

	void OnScrollBar(float pos);

	// KeyboardSink
	bool OnKeyPressed(InputContext &ic, Key key) override;

	// ScrollSink
	void OnScroll(TextureManager &texman, const InputContext &ic, const LayoutContext &lc, const DataContext &dc, vec2d pointerPosition, vec2d scrollOffset) override;
};

} // namespace UI
