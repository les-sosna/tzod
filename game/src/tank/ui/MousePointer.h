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
public:
	MouseCursor(GuiManager* manager, const char *texture);
	virtual void Draw(float sx = 0, float sy = 0);
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
