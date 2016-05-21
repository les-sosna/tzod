#pragma once
#include "Pointers.h"
#include <math/MyMath.h>
#include <memory>
#include <unordered_map>

namespace UI
{

enum class Key;
struct IInput;
struct IClipboard;
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
		vec2d size,
		vec2d pointerPosition,
		float z,
		Msg msg,
		int button,
		PointerType pointerType,
		unsigned int pointerID);
	bool ProcessKeys(std::shared_ptr<Window> wnd, Msg msg, UI::Key key);
	bool ProcessText(std::shared_ptr<Window> wnd, int c);

	IClipboard &GetClipboard() const { return _clipboard; }
	IInput& GetInput() const { return _input; }

	std::shared_ptr<Window> GetCapture(unsigned int pointerID) const;
	void SetCapture(unsigned int pointerID, std::shared_ptr<Window> wnd);
	bool HasCapturedPointers(Window* wnd) const;

	bool GetMainWindowActive() const { return _isAppActive; }

#ifndef NDEBUG
	const std::unordered_map<unsigned int, vec2d>& GetLastPointerLocation() const { return _lastPointerLocation; }
#endif

	void ResetWindow(Window &wnd);

private:
	bool ProcessKeyPressedRecursive(std::shared_ptr<Window> wnd, Key key);
	bool ProcessCharRecursive(std::shared_ptr<Window> wnd, int c);

	bool ProcessPointerInternal(
		vec2d size,
		std::shared_ptr<Window> wnd,
		vec2d pointerPosition,
		float z,
		Msg msg,
		int buttons,
		PointerType pointerType,
		unsigned int pointerID,
		bool topMostPass,
		bool insideTopMost = false);

	IInput &_input;
	IClipboard &_clipboard;

	struct PointerCapture
	{
		unsigned int captureCount = 0;
		std::weak_ptr<Window> captureWnd;
	};

	std::unordered_map<unsigned int, PointerCapture> _pointerCaptures;

	unsigned int _captureCountSystem;

	std::weak_ptr<Window> _hotTrackWnd;

	bool _isAppActive;
#ifndef NDEBUG
	std::unordered_map<unsigned int, vec2d> _lastPointerLocation;
#endif
};

} // namespace UI
