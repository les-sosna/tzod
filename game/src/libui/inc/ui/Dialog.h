#pragma once
#include "Window.h"
#include <functional>

namespace UI
{
	// TODO: to make dialog modal create a full screen window behind it

class Dialog : public Window
{
public:
	Dialog(Window *parent, float width, float height, bool modal = true);

	void SetEasyMove(bool enable);

    enum
    {
        _resultOK,
        _resultCancel
    };

	std::function<void(int)> eventClose;

	void Close(int result);

protected:
	// Window
	bool OnMouseDown(float x, float y, int button) override;
	bool OnMouseUp(float x, float y, int button) override;
	bool OnMouseMove(float x, float y) override;
	bool OnMouseEnter(float x, float y) override;
	bool OnMouseLeave() override;
	bool OnKeyPressed(Key key) override;
	bool OnFocus(bool focus) override;

private:
	float _mouseX;
	float _mouseY;
	bool  _easyMove;

	virtual bool OnClose(int result) { return true; }
};

} // end of namespace UI
