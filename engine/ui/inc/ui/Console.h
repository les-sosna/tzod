#pragma once
#include "Window.h"
#include "PointerInput.h"
#include <deque>
#include <functional>
#include <string>
#include <vector>

class TextureManager;

namespace Plat
{
	class ConsoleBuffer;
}

namespace UI
{

class Edit;
class Rectangle;
class ScrollBarVertical;

struct IConsoleHistory
{
	virtual void Enter(std::string str) = 0;
	virtual size_t GetItemCount() const = 0;
	virtual std::string_view GetItem(size_t index) const = 0;
};

class ConsoleHistoryDefault : public IConsoleHistory
{
public:
	ConsoleHistoryDefault(size_t maxSize);

	void Enter(std::string str) override;
	size_t GetItemCount() const override;
	std::string_view GetItem(size_t index) const override;

private:
	size_t _maxSize;
	std::deque<std::string> _buf;
};

///////////////////////////////////////////////////////////////////////////////

class Console
	: public WindowContainer
	, private TimeStepping
	, private ScrollSink
	, private KeyboardSink
	, private PointerSink
{
public:
	Console(TimeStepManager &manager, TextureManager &texman);
	static std::shared_ptr<Console> Create(Window *parent, TimeStepManager &manager, TextureManager &texman, float x, float y, float w, float h, Plat::ConsoleBuffer *buf);

	void SetColors(const SpriteColor *colors, size_t count);
	void SetHistory(IConsoleHistory *history);
	void SetBuffer(Plat::ConsoleBuffer *buf);
	void SetEcho(bool echo);
	std::function<void(std::string_view)> eventOnSendCommand;
	std::function<bool(std::string_view, int &, std::string &)> eventOnRequestCompleteCommand;

	// Window
	ScrollSink* GetScrollSink() override { return this; }
	KeyboardSink *GetKeyboardSink() override { return this; }
	PointerSink* GetPointerSink() override { return this; }

	void Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, const Plat::Input &input, float time, bool hovered) const override;
	WindowLayout GetChildLayout(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const override;
	std::shared_ptr<const Window> GetFocus(const std::shared_ptr<const Window>& owner) const override;
	const Window* GetFocus() const override;

	// TimeStepping
	void OnTimeStep(const Plat::Input &input, bool focused, float dt) override;

private:
	std::shared_ptr<Rectangle> _background;
	std::shared_ptr<ScrollBarVertical> _scroll;
	std::shared_ptr<Edit> _input;
	size_t _cmdIndex;
	size_t _font;

	Plat::ConsoleBuffer *_buf;
	IConsoleHistory *_history;
	std::vector<SpriteColor> _colors;

	bool _echo;
	bool _autoScroll;

	void OnScrollBar(float pos);

	// KeyboardSink
	bool OnKeyPressed(const Plat::Input &input, const InputContext &ic, Plat::Key key) override;

	// ScrollSink
	void OnScroll(TextureManager &texman, const InputContext &ic, const LayoutContext &lc, const DataContext &dc, vec2d scrollOffset, bool precise) override;
	void EnsureVisible(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, FRECT pxFocusRect) override {}
};

} // namespace UI
