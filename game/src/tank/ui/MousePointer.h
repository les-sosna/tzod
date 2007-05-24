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
	MouseCursor(GuiManager* manager, const char *texture);
	virtual void Draw(float sx = 0, float sy = 0);

protected:
	void OnTimeStep(float dt);
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
