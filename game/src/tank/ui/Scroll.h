// Scroll.h

#pragma once

#include "Window.h"

namespace UI
{
// forward declarations
class ImageButton;

///////////////////////////////////////////////////////////////////////////////
// vertical scrollbar control

class ScrollBar : public Window
{
    ImageButton *_btnUp;
    ImageButton *_btnDown;
    ImageButton *_btnBox;

    float _tmpBoxY;

	float _pos;
	float _lineSize;
	float _limit;

public:
	ScrollBar(Window *parent, float x, float y, float height);

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

	void OnUp();
	void OnDown();
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file