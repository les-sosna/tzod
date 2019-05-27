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

namespace Plat
{
	enum class Key;
	struct Input;
}

namespace UI
{

class DataContext;
class InputContext;
class LayoutContext;
class StateContext;
struct LayoutConstraints;
struct NavigationSink;
struct PointerSink;
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

struct ScrollSink
{
	virtual void OnScroll(TextureManager &texman, const InputContext &ic, const LayoutContext &lc, const DataContext &dc, vec2d scrollOffset, bool precise) = 0;
	virtual void EnsureVisible(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, FRECT pxFocusRect) = 0;
};

struct KeyboardSink
{
	virtual bool OnKeyPressed(const InputContext &ic, Plat::Key key) { return false; }
	virtual void OnKeyReleased(const InputContext &ic, Plat::Key key) {}
};

struct TextSink
{
	virtual bool OnChar(int c) = 0;
	virtual void OnPaste(std::string_view text) = 0;
	virtual std::string_view OnCopy() const = 0;
	virtual std::string OnCut() = 0;
};

class StateContext;

struct StateGen
{
	virtual void PushState(StateContext &sc, const LayoutContext &lc, const InputContext &ic, bool hovered) const = 0;
};

class Window
{
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

	virtual unsigned int GetChildrenCount() const { return static_cast<unsigned int>(_children.size()); }
	virtual std::shared_ptr<const Window> GetChild(unsigned int index) const { return _children[index]; }
	std::shared_ptr<Window> GetChild(unsigned int index)
	{
		return std::const_pointer_cast<Window>(static_cast<const Window*>(this)->GetChild(index));
	}

	virtual FRECT GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const;
	virtual float GetChildOpacity(const LayoutContext& lc, const InputContext& ic, const Window &child) const { return 1; }
	virtual bool GetChildEnabled(const Window& child) const { return true; }

	//
	// Input
	//
	virtual bool HasNavigationSink() const { return false; }
	virtual NavigationSink* GetNavigationSink() { return nullptr; }
	virtual bool HasScrollSink() const { return false; }
	virtual ScrollSink* GetScrollSink() { return nullptr; }
	virtual bool HasPointerSink() const { return false; }
	virtual PointerSink* GetPointerSink() { return nullptr; }
	virtual bool HasKeyboardSink() const { return false; }
	virtual KeyboardSink* GetKeyboardSink() { return nullptr; }
	virtual bool HasTextSink() const { return false; }
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

	void SetClipChildren(bool clip) { _clipChildren = clip; }
	bool GetClipChildren() const { return _clipChildren; }


	//
	// size & position
	//

	virtual vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const { return Vec2dFloor(GetSize() *scale); }

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

	void SetFocus(std::shared_ptr<Window> child);
	virtual std::shared_ptr<Window> GetFocus() const;

	// rendering
	virtual void Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, float time, bool hovered) const {}

private:
	std::shared_ptr<Window> _focusChild;
	std::deque<std::shared_ptr<Window>> _children;

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
		bool _isVisible : 1;
		bool _isTopMost : 1;
		bool _clipChildren : 1;
	};
};

inline bool NeedsFocus(const Window *wnd)
{
	if (!wnd || !wnd->GetVisible())
		return false;

	if (wnd->HasNavigationSink() || wnd->HasKeyboardSink() || wnd->HasTextSink())
		return true;

	if (auto focus = wnd->GetFocus().get())
		if (wnd->GetChildEnabled(*focus) && NeedsFocus(focus))
			return true;

	return false;
}

FRECT CanvasLayout(vec2d offset, vec2d size, float scale);

//////////////////////// to remove ////////////////////
class TimeStepManager;
class Managerful
{
protected:
	explicit Managerful(TimeStepManager &manager) : _manager(manager) {}

	TimeStepManager& GetTimeStepManager() const { return _manager; }

private:
	TimeStepManager &_manager;
};

class TimeStepping : public Managerful
{
public:
	explicit TimeStepping(TimeStepManager &manager) : Managerful(manager) {}
	~TimeStepping();

	void SetTimeStep(bool enable);

	virtual void OnTimeStep(Plat::Input &input, bool focused, float dt) {}

private:
	std::list<TimeStepping*>::iterator _timeStepReg;
	bool _isTimeStep = false;
};

} // namespace UI
