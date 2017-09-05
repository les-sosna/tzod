#pragma once
#include "Pointers.h"
#include <math/MyMath.h>
#include <memory>
#include <stack>
#include <unordered_map>
#include <vector>

class TextureManager;

namespace UI
{

class DataContext;
class LayoutContext;
class Window;
enum class Key;
struct IInput;
struct IClipboard;
struct PointerSink;

enum class Msg
{
	KeyReleased,
	KeyPressed,
	PointerDown,
	PointerMove,
	PointerUp,
	PointerCancel,
	Scroll,
	TAP,
};

class InputContext
{
public:
	InputContext(IInput &input, IClipboard &clipboard);

	bool ProcessPointer(
		TextureManager &texman,
		std::shared_ptr<Window> wnd,
		const LayoutContext &lc,
		const DataContext &dc,
		vec2d pxPointerPosition,
		vec2d pxPointerOffset,
		Msg msg,
		int button,
		PointerType pointerType,
		unsigned int pointerID);
	bool ProcessKeys(TextureManager &texman, std::shared_ptr<Window> wnd, const LayoutContext &lc, const DataContext &dc, Msg msg, UI::Key key, float time);
	bool ProcessText(TextureManager &texman, std::shared_ptr<Window> wnd, const LayoutContext &lc, const DataContext &dc, int c);
	bool ProcessSystemNavigationBack(std::shared_ptr<Window> wnd, const DataContext &dc);

	IClipboard &GetClipboard() const { return _clipboard; }
	IInput& GetInput() const { return _input; }

	void PushInputTransform(vec2d offset, bool focused, bool hovered);
	void PopInputTransform();

	vec2d GetMousePos() const;
	bool GetFocused() const;
	bool GetHovered() const;
	float GetLastKeyTime() const { return _lastKeyTime; }

	const std::vector<std::shared_ptr<Window>>* GetCapturePath(unsigned int pointerID) const;
	bool HasCapturedPointers(const Window* wnd) const;

	bool GetMainWindowActive() const { return _isAppActive; }

#ifndef NDEBUG
	const std::unordered_map<unsigned int, vec2d>& GetLastPointerLocation() const { return _lastPointerLocation; }
#endif

private:
	bool ProcessScroll(TextureManager &texman, std::shared_ptr<Window> wnd, const LayoutContext &lc, const DataContext &dc, vec2d pxPointerPosition, vec2d offset);

	IInput &_input;
	IClipboard &_clipboard;

	struct InputStackFrame
	{
		vec2d offset;
		bool focused;
		bool hovered;
	};
	std::stack<InputStackFrame> _transformStack;

	float _lastKeyTime = 0;

	struct PointerCapture
	{
		std::vector<std::shared_ptr<Window>> capturePath;
	};

	std::unordered_map<unsigned int, PointerCapture> _pointerCaptures;

	bool _isAppActive;
#ifndef NDEBUG
	std::unordered_map<unsigned int, vec2d> _lastPointerLocation;
#endif
};

class LayoutContext;

struct AreaSinkSearch
{
	TextureManager &texman;
	const DataContext &dc;
	bool topMostPass;
	std::vector<std::shared_ptr<Window>> outSinkPath;
};

template<class SinkType>
SinkType* FindAreaSink(
	AreaSinkSearch &search,
	std::shared_ptr<Window> wnd,
	const LayoutContext &lc,
	vec2d pxPointerPosition,
	bool insideTopMost);

} // namespace UI
