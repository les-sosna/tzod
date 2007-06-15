// GuiManager.h

#pragma once

#include "Base.h"


// forward declarations
class GuiManager;

typedef UI::Window* (*CreateWindowProc) (GuiManager *);

///////////////////////////////////////////////////////////////////////////////

class GuiManager
{
	UI::Window* _desktop;
	UI::Window* _cursor;

	UI::Window* _focusWnd;
	UI::Window* _hotTrackWnd;
	UI::Window* _captureWnd;

	PtrList<UI::Window>    _timestep;
	std::list<UI::Window*> _topmost;
	std::stack<RECT>       _clipStack;

	int _captureCount;
	int _windowCount;

	bool _ProcessMouse(UI::Window* wnd, float x, float y, float z, UINT msg);

public:
	void Add(UI::Window* wnd);
	void Remove(UI::Window* wnd);

	UI::Window* GetCapture() const;
	void SetCapture(UI::Window* wnd);
	void ReleaseCapture(UI::Window* wnd);

	void AddTopMost(UI::Window* wnd, bool add);

	void PushClippingRect(const RECT &rect);
	void PopClippingRect();

	bool SetFocusWnd(UI::Window* wnd);
	UI::Window* GetFocusWnd() const;

public:
	PtrList<UI::Window>::iterator TimeStepRegister(UI::Window* wnd);
	void TimeStepUnregister(PtrList<UI::Window>::iterator it);
	void TimeStep(float dt);

public:
	GuiManager(CreateWindowProc createDesctop);
	~GuiManager();

	bool ProcessMouse(float x, float y, float z, UINT msg);
	bool ProcessKeys(UINT msg, int c);

	UI::Window* GetDesktop() const;

	int GetWndCount() const;

	void Render() const;
	void Show(bool show);

	void Resize(float width, float height);
};

// end of file
