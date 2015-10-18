// GuiManager.h

#pragma once

#include "Window.h"
#include <list>

class TextureManager;

namespace UI
{

class LayoutManager;
struct IInput;
struct IClipboard;

struct IWindowFactory
{
    virtual Window* Create(LayoutManager *pManager) = 0;
};

enum Msg
{
    MSGKEYUP,
    MSGKEYDOWN,
    MSGCHAR,
	MSGLBUTTONDOWN,
	MSGRBUTTONDOWN,
	MSGMBUTTONDOWN,
	MSGLBUTTONUP,
	MSGRBUTTONUP,
	MSGMBUTTONUP,
	MSGMOUSEMOVE,
	MSGMOUSEWHEEL,
};

class LayoutManager
{
public:
	LayoutManager(IInput &input, IClipboard &clipboard, TextureManager &texman, IWindowFactory &&desktopFactory);
	~LayoutManager();

	void TimeStep(float dt);
	void Render(DrawingContext &dc) const;

	bool ProcessMouse(float x, float y, float z, Msg msg);
	bool ProcessKeys(Msg msg, int c);

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

	bool ProcessMouseInternal(Window* wnd, float x, float y, float z, Msg msg);

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
