// MousePointer.h

#pragma once

#include "Base.h"
#include "Window.h"


namespace UI
{

///////////////////////////////////////////////////////////////////////////////

class MouseCursor : public Window
{
	Text *_text;
	float _timeShow;
	float _timeAnim;

public:
	MouseCursor(LayoutManager* manager, const char *texture);

protected:
	void OnTimeStep(float dt);
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
