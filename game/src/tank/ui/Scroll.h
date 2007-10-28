// Scroll.h

#pragma once

#include "Base.h"
#include "Window.h"

namespace UI
{

///////////////////////////////////////////////////////////////////////////////
// vertical scrollbar control

class ScrollBar : public Window
{
    ImageButton *_btnUpLeft;
    ImageButton *_btnDownRight;
    ImageButton *_btnBox;

    float _tmpBoxPos;

	float _pos;
	float _lineSize;
	float _limit;

	bool _hor;

public:
	ScrollBar(Window *parent, float x, float y, float size, bool hor = false);

	void  SetPos(float pos);
	float GetPos() const;

	void  SetLimit(float limit);
	float GetLimit() const;

	void  SetLineSize(float ls);

	Delegate<void(float)> eventScroll;

protected:
	void OnSize(float width, float height);

private:
	void OnBoxMouseDown(float x, float y);
	void OnBoxMouseUp(float x, float y);
	void OnBoxMouseMove(float x, float y);

	void OnUpLeft();
	void OnDownRight();
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file