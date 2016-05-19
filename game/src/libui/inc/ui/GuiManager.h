#pragma once
#include "Pointers.h"
#include <math/MyMath.h>
#include <list>
#include <memory>
#include <unordered_map>

class DrawingContext;
class TextureManager;

namespace UI
{

class Window;
class LayoutManager;
enum class Key;
struct IInput;
struct IClipboard;

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

class LayoutManager
{
public:
	LayoutManager(IInput &input, IClipboard &clipboard, TextureManager &texman);
	~LayoutManager();

	void TimeStep(float dt);
	void Render(FRECT rect, DrawingContext &dc) const;

	bool ProcessPointer(vec2d size, float x, float y, float z, Msg msg, int button, PointerType pointerType, unsigned int pointerID);
	bool ProcessKeys(Msg msg, UI::Key key);
	bool ProcessText(int c);

	IClipboard &GetClipboard() const { return _clipboard; }
	IInput& GetInput() const { return _input; }
	TextureManager& GetTextureManager() const { return _texman; }
	float GetTime() const { return _time; }

	Window* GetDesktop() const;
	void SetDesktop(std::shared_ptr<Window> desktop) { _desktop = std::move(desktop); }

	std::shared_ptr<Window> GetCapture(unsigned int pointerID) const;
	void SetCapture(unsigned int pointerID, std::shared_ptr<Window> wnd);
	bool HasCapturedPointers(Window* wnd) const;

	bool IsMainWindowActive() const { return _isAppActive; }

private:
	friend class Window;
	void ResetWindow(Window &wnd);
	std::list<Window*>::iterator TimeStepRegister(Window* wnd);
	void TimeStepUnregister(std::list<Window*>::iterator it);

	static bool ProcessKeyPressedRecursive(std::shared_ptr<Window> wnd, Key key);
	static bool ProcessCharRecursive(std::shared_ptr<Window> wnd, int c);

	bool ProcessPointerInternal(
		vec2d size,
		std::shared_ptr<Window> wnd,
		float x,
		float y,
		float z,
		Msg msg,
		int buttons,
		PointerType pointerType,
		unsigned int pointerID,
		bool topMostPass,
		bool insideTopMost = false);

	IInput &_input;
	IClipboard &_clipboard;
	TextureManager &_texman;
	std::list<Window*> _timestep;
	std::list<Window*>::iterator _tsCurrent;
	bool _tsDeleteCurrent;
	float _time = 0;

	struct PointerCapture
	{
		unsigned int captureCount = 0;
		std::weak_ptr<Window> captureWnd;
	};

	std::unordered_map<unsigned int, PointerCapture> _pointerCaptures;

	unsigned int _captureCountSystem;

	std::weak_ptr<Window> _hotTrackWnd;

	std::shared_ptr<Window> _desktop;

	bool _isAppActive;
#ifndef NDEBUG
	bool _dbgFocusIsChanging;
	std::unordered_map<unsigned int, vec2d> _lastPointerLocation;
#endif
};

} // namespace UI
