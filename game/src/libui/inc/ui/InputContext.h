#pragma once
#include "Pointers.h"
#include <math/MyMath.h>
#include <memory>
#include <stack>
#include <unordered_map>
#include <vector>

namespace UI
{

enum class Key;
struct IInput;
struct IClipboard;
struct PointerSink;
class Window;

enum class Msg
{
	KEYUP,
	KEYDOWN,
	PointerDown,
	PointerMove,
	PointerUp,
	PointerCancel,
	MOUSEWHEEL,
	TAP,
};

class InputContext
{
public:
	InputContext(IInput &input, IClipboard &clipboard);

	bool ProcessPointer(
		std::shared_ptr<Window> wnd,
		float layoutScale,
		vec2d pxSize,
		vec2d pxPointerPosition,
		float z,
		Msg msg,
		int button,
		PointerType pointerType,
		unsigned int pointerID);
	bool ProcessKeys(std::shared_ptr<Window> wnd, Msg msg, UI::Key key);
	bool ProcessText(std::shared_ptr<Window> wnd, int c);

	IClipboard &GetClipboard() const { return _clipboard; }
	IInput& GetInput() const { return _input; }

	void PushTransform(vec2d offset, bool focused, bool hovered);
	void PopTransform();

	vec2d GetMousePos() const;
	bool GetFocused() const;
	bool GetHovered() const;

	const std::vector<std::shared_ptr<Window>>* GetCapturePath(unsigned int pointerID) const;
	bool HasCapturedPointers(const Window* wnd) const;

	bool GetMainWindowActive() const { return _isAppActive; }

#ifndef NDEBUG
	const std::unordered_map<unsigned int, vec2d>& GetLastPointerLocation() const { return _lastPointerLocation; }
#endif

private:
	bool ProcessCharRecursive(std::shared_ptr<Window> wnd, int c);
	bool ProcessKeyPressedRecursive(std::shared_ptr<Window> wnd, Key key);
	bool ProcessScroll(std::shared_ptr<Window> wnd, vec2d offset, vec2d pxSize, vec2d pxPointerPosition, float layoutScale);

	IInput &_input;
	IClipboard &_clipboard;

	struct InputStackFrame
	{
		vec2d offset;
		bool focused;
		bool hovered;
	};
	std::stack<InputStackFrame> _transformStack;

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


struct AreaSinkSearch
{
	float layoutScale;
	bool topMostPass;
	std::vector<std::shared_ptr<Window>> outSinkPath;
};

template<class SinkType>
SinkType* FindAreaSink(
	AreaSinkSearch &search,
	std::shared_ptr<Window> wnd,
	vec2d pxSize,
	vec2d pxPointerPosition,
	bool insideTopMost);

} // namespace UI
