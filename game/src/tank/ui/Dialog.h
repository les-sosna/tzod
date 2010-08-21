// Dialog.h

#pragma once

#include "Window.h"


namespace UI
{
	// TODO: to make dialog modal create a full screen window under it

class Dialog : public Window
{
public:
	Dialog(Window *parent, float width, float height, bool modal = true);

	void SetEasyMove(bool enable);

	static const int _resultOK     = 0;
	static const int _resultCancel = 1;
	virtual bool OnRawChar(int c);
	std::tr1::function<void(int)> eventClose;

protected:
	void Close(int result);

	virtual bool OnMouseDown (float x, float y, int button);
	virtual bool OnMouseUp   (float x, float y, int button);
	virtual bool OnMouseMove (float x, float y);
	virtual bool OnMouseEnter(float x, float y);
	virtual bool OnMouseLeave();

	
	virtual bool OnFocus(bool focus);

private:
	float _mouseX;
	float _mouseY;
	bool  _easyMove;
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
