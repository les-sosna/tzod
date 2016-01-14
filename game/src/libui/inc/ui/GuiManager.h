// GuiManager.h

#pragma once

#include "WindowPtr.h"
#include <list>

class DrawingContext;
class TextureManager;

namespace UI
{

class Window;
class LayoutManager;
enum class Key;
struct IInput;
struct IClipboard;

struct IWindowFactory
{
    virtual Window* Create(LayoutManager *pManager) = 0;
};

enum class Msg
{
    KEYUP,
    KEYDOWN,
	LBUTTONDOWN,
	RBUTTONDOWN,
	MBUTTONDOWN,
	LBUTTONUP,
	RBUTTONUP,
	MBUTTONUP,
	MOUSEMOVE,
	MOUSEWHEEL,
    TAP,
};

class LayoutManager
{
public:
	LayoutManager(IInput &input, IClipboard &clipboard, TextureManager &texman, IWindowFactory &&desktopFactory);
	~LayoutManager();

	void TimeStep(float dt);
	void Render(DrawingContext &dc) const;

	bool ProcessPointer(float x, float y, float z, Msg msg);
	bool ProcessKeys(Msg msg, UI::Key key);
	bool ProcessText(int c);

	IClipboard &GetClipboard() const { return _clipboard; }
	IInput& GetInput() const { return _input; }
	TextureManager& GetTextureManager() const { return _texman; }
	Window* GetDesktop() const;

	Window* GetCapture() const;
	void SetCapture(Window* wnd);

	bool SetFocusWnd(Window* wnd);  // always resets previous focus
	Window* GetFocusWnd() const;
//	bool ResetFocus(Window* wnd);   // remove focus from wnd or any of its children

	bool IsMainWindowActive() const { return _isAppActive; }

private:
	friend class Window;
	void AddTopMost(Window* wnd, bool add);
	void ResetWindow(Window* wnd);
    std::list<Window*>::iterator TimeStepRegister(Window* wnd);
	void TimeStepUnregister(std::list<Window*>::iterator it);

	bool ProcessPointerInternal(Window* wnd, float x, float y, float z, Msg msg);

	IInput &_input;
	IClipboard &_clipboard;
    TextureManager &_texman;
    std::list<Window*> _topmost;
    std::list<Window*> _timestep;
    std::list<Window*>::iterator _tsCurrent;
    bool _tsDeleteCurrent;


	unsigned int _captureCountSystem;
	unsigned int _captureCount;

	WindowWeakPtr _focusWnd;
	WindowWeakPtr _hotTrackWnd;
	WindowWeakPtr _captureWnd;

	WindowWeakPtr _desktop;

	bool _isAppActive;
#ifndef NDEBUG
	bool _dbgFocusIsChanging;
#endif
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI
// end of file
