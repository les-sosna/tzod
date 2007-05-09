// gui_desktop.h

#pragma once

#include "gui_base.h"

namespace UI
{

///////////////////////////////////////////////////////////////////////////////

class Desktop : public Window
{
public:
	Desktop(GuiManager* manager);

protected:
	virtual void OnRawChar(int c);
	virtual bool OnFocus(bool focus);
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
