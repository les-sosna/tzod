#pragma once
#include "WindowPtr.h"
#include "Pointers.h"
#include <math/MyMath.h>
#include <list>
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

struct IWindowFactory
{
    virtual Window* Create(LayoutManager *pManager) = 0;
};

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
	LayoutManager(IInput &input, IClipboard &clipboard, TextureManager &texman, IWindowFactory &&desktopFactory);
	~LayoutManager();

	void TimeStep(float dt);
	void Render(DrawingContext &dc) const;

	bool ProcessPointer(float x, float y, float z, Msg msg, int button, PointerType pointerType, unsigned int pointerID);
	bool ProcessKeys(Msg msg, UI::Key key);
	bool ProcessText(int c);

	IClipboard &GetClipboard() const { return _clipboard; }
	IInput& GetInput() const { return _input; }
	TextureManager& GetTextureManager() const { return _texman; }
	Window* GetDesktop() const;

	Window* GetCapture(unsigned int pointerID) const;
	void SetCapture(unsigned int pointerID, Window* wnd);
    bool HasCapturedPointers(Window* wnd) const;

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

	bool ProcessPointerInternal(Window* wnd, float x, float y, float z, Msg msg, int buttons, PointerType pointerType, unsigned int pointerID);

	IInput &_input;
	IClipboard &_clipboard;
    TextureManager &_texman;
    std::list<Window*> _topmost;
    std::list<Window*> _timestep;
    std::list<Window*>::iterator _tsCurrent;
    bool _tsDeleteCurrent;
    
    struct PointerCapture
    {
        unsigned int captureCount = 0;
        WindowWeakPtr captureWnd;
    };
    
    std::unordered_map<unsigned int, PointerCapture> _pointerCaptures;


	unsigned int _captureCountSystem;

	WindowWeakPtr _focusWnd;
	WindowWeakPtr _hotTrackWnd;

	WindowWeakPtr _desktop;

	bool _isAppActive;
#ifndef NDEBUG
	bool _dbgFocusIsChanging;
    std::unordered_map<unsigned int, vec2d> _lastPointerLocation;
#endif
};

} // namespace UI
