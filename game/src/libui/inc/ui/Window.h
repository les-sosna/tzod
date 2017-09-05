#pragma once

#include <video/RenderBase.h>

#include <cassert>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <deque>

class RenderContext;
class TextureManager;

namespace UI
{

class DataContext;
class InputContext;
class LayoutContext;
class StateContext;
enum class Key;
enum class PointerType;
template <class> struct LayoutData;

enum class StretchMode
{
	Stretch,
	Fill,
};

enum class FlowDirection
{
	Vertical,
	Horizontal
};

enum class Navigate
{
	None,
	Enter,
	Back,
	Prev,
	Next,
	Up,
	Down,
	Left,
	Right,
	Begin,
	End
};

struct NavigationSink
{
	virtual bool CanNavigate(Navigate navigate, const DataContext &dc) const = 0;
	virtual void OnNavigate(Navigate navigate, const DataContext &dc) = 0;
};

struct ScrollSink
{
	virtual void OnScroll(TextureManager &texman, const InputContext &ic, const LayoutContext &lc, const DataContext &dc, vec2d pointerPosition, vec2d scrollOffset) = 0;
};

struct PointerSink
{
	virtual bool OnPointerDown(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID) { return false; }
	virtual void OnPointerUp(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID) {}
	virtual void OnPointerMove(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, PointerType pointerType, unsigned int pointerID, bool captured) {}
	virtual void OnTap(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition) {}
};

struct KeyboardSink
{
	virtual bool OnKeyPressed(InputContext &ic, Key key) { return false; }
};

struct TextSink
{
	virtual bool OnChar(int c) = 0;
};

class StateContext;

struct StateGen
{
	virtual void PushState(StateContext &sc, const LayoutContext &lc, const InputContext &ic) const = 0;
};

class Window : public std::enable_shared_from_this<Window>
{
	std::shared_ptr<Window> _focusChild;
	std::deque<std::shared_ptr<Window>> _children;

	std::shared_ptr<LayoutData<bool>> _enabled;

	//
	// size and position
	//

	float _x = 0;
	float _y = 0;
	float _width = 0;
	float _height = 0;


	//
	// attributes
	//

	struct
	{
		bool _isVisible      : 1;
		bool _isTopMost      : 1;
		bool _clipChildren   : 1;
	};

public:
	Window();
	virtual ~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	void UnlinkAllChildren();
	void UnlinkChild(Window &child);
	void AddFront(std::shared_ptr<Window> child);
	void AddBack(std::shared_ptr<Window> child);

	const std::deque<std::shared_ptr<Window>>& GetChildren() const { return _children; }

	virtual FRECT GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const;
	virtual float GetChildOpacity(const Window &child) const { return 1; }

	//
	// Input
	//
	virtual NavigationSink* GetNavigationSink() { return nullptr; }
	virtual ScrollSink* GetScrollSink() { return nullptr; }
	virtual PointerSink* GetPointerSink() { return nullptr; }
	virtual KeyboardSink* GetKeyboardSink() { return nullptr; }
	virtual TextSink* GetTextSink() { return nullptr; }

	// State
	virtual const StateGen* GetStateGen() const { return nullptr; }

	//
	// Appearance
	//

	void SetVisible(bool show);
	bool GetVisible() const { return _isVisible; }

	void SetTopMost(bool topmost);
	bool GetTopMost() const { return _isTopMost; }

	void SetClipChildren(bool clip)  { _clipChildren = clip; }
	bool GetClipChildren() const     { return _clipChildren; }


	//
	// size & position
	//

	virtual vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale) const { return Vec2dFloor(GetSize() *scale); }

	void Move(float x, float y);
	vec2d GetOffset() const { return vec2d{_x, _y}; }

	void Resize(float width, float height);
	void SetHeight(float height) { Resize(GetWidth(), height); }
	void SetWidth(float width) { Resize(width, GetHeight()); }
	float GetWidth() const { return _width; }
	float GetHeight() const { return _height; }
	vec2d GetSize() const { return vec2d{GetWidth(), GetHeight()}; }


	//
	// Behavior
	//

	void SetEnabled(std::shared_ptr<LayoutData<bool>> enabled);
	bool GetEnabled(const DataContext &dc) const;

	void SetFocus(std::shared_ptr<Window> child);
	std::shared_ptr<Window> GetFocus() const;


	//
	// Events
	//
	std::function<void(void)> eventLostFocus;


	//
	// rendering
	//

	virtual void Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const {}
};

inline bool NeedsFocus(Window *wnd, const DataContext &dc)
{
	return (wnd && wnd->GetVisible() && wnd->GetEnabled(dc)) ?
		wnd->GetNavigationSink() || wnd->GetKeyboardSink() || wnd->GetTextSink() || NeedsFocus(wnd->GetFocus().get(), dc) : false;
}

FRECT CanvasLayout(vec2d offset, vec2d size, float scale);

//////////////////////// to remove ////////////////////
class LayoutManager;
class Managerful
{
protected:
	explicit Managerful(LayoutManager &manager) : _manager(manager) {}

	LayoutManager& GetManager() const { return _manager; }

private:
	LayoutManager &_manager;
};

class TimeStepping : public Managerful
{
public:
	explicit TimeStepping(LayoutManager &manager) : Managerful(manager) {}
	~TimeStepping();

	void SetTimeStep(bool enable);

	virtual void OnTimeStep(LayoutManager &manager, float dt) {}

private:
	std::list<TimeStepping*>::iterator _timeStepReg;
	bool _isTimeStep = false;
};

} // namespace UI
