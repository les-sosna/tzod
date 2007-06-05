// Dialog.h

#pragma once

#include "Window.h"


namespace UI
{
	// TODO: to make dialog modal create a full screen window under it

class Dialog : public Window
{
	bool  _easyMove;
	float _mouseX;
	float _mouseY;

public:
	Dialog(Window *parent, float x, float y, float width, float height,
		bool modal = true);
	virtual ~Dialog();

	void SetEasyMove(bool enable);

    static const int _resultOK     = 0;
	static const int _resultCancel = 1;

	Delegate<void(int)> eventClose;

protected:
	void Close(int result);

	virtual bool OnMouseDown (float x, float y, int button);
	virtual bool OnMouseUp   (float x, float y, int button);
	virtual bool OnMouseMove (float x, float y);
	virtual bool OnMouseEnter(float x, float y);
	virtual bool OnMouseLeave();

	virtual void OnRawChar(int c);
	virtual bool OnFocus(bool focus);
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
