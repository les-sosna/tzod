#pragma once

#include <video/RenderBase.h>

#include <cassert>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <deque>

class DrawingContext;
class TextureManager;

namespace UI
{

class InputContext;
class LayoutContext;
class LayoutManager;
enum class Key;
enum class PointerType;

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

struct ScrollSink
{
	virtual void OnScroll(InputContext &ic, LayoutContext &lc, vec2d pointerPosition, vec2d scrollOffset) = 0;
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
	friend class LayoutManager;
	LayoutManager &_manager;

	std::shared_ptr<Window> _focusChild;
	std::deque<std::shared_ptr<Window>> _children;

	std::list<Window*>::iterator _timeStepReg;


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
		bool _isEnabled      : 1;
		bool _isTopMost      : 1;
		bool _isTimeStep     : 1;
		bool _clipChildren   : 1;
	};

public:
	explicit Window(LayoutManager &manager);
	virtual ~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	void UnlinkAllChildren();
	void UnlinkChild(Window &child);
	void AddFront(std::shared_ptr<Window> child);
	void AddBack(std::shared_ptr<Window> child);

	const std::deque<std::shared_ptr<Window>>& GetChildren() const { return _children; }
	LayoutManager& GetManager() const { return _manager;  } // to remove

	virtual FRECT GetChildRect(const LayoutContext &lc, const Window &child) const;

	//
	// Input
	//
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

	virtual vec2d GetContentSize(const StateContext &sc, TextureManager &texman) const { return GetSize(); }

	void Move(float x, float y);
	vec2d GetOffset() const { return vec2d{_x, _y}; }

	void Resize(float width, float height);
	void SetHeight(float height) { Resize(GetWidth(), height); }
	void SetWidth(float width) { Resize(width, GetHeight()); }
	virtual float GetWidth() const { return _width; }
	virtual float GetHeight() const { return _height; }
	vec2d GetSize() const { return vec2d{GetWidth(), GetHeight()}; }


	//
	// Behavior
	//

	void SetEnabled(bool enable);
	bool GetEnabled() const { return _isEnabled; }

	void SetTimeStep(bool enable);
	bool GetTimeStep() const { return _isTimeStep; }

	void SetFocus(std::shared_ptr<Window> child);
	std::shared_ptr<Window> GetFocus() const;

	//
	// Events
	//
	std::function<void(void)> eventLostFocus;


	//
	// rendering
	//

	virtual void Draw(const StateContext &sc, const LayoutContext &lc, const InputContext &ic, DrawingContext &dc, TextureManager &texman) const {}

private:

	//
	// size
	//

	virtual void OnSize(float width, float height);


	//
	// other
	//

	virtual void OnTimeStep(LayoutManager &manager, float dt);
};

inline bool NeedsFocus(Window *wnd)
{
	return wnd ? wnd->GetKeyboardSink() || wnd->GetTextSink() || NeedsFocus(wnd->GetFocus().get()) : false;
}

FRECT CanvasLayout(vec2d offset, vec2d size, float scale);

} // namespace UI
