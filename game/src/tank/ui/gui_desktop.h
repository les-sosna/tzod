// gui_desktop.h

#pragma once

#include "Window.h"

namespace UI
{

///////////////////////////////////////////////////////////////////////////////

class Desktop : public Window
{
public:
	Desktop(GuiManager* manager);

	void ShowDesktopBackground(bool show);
	
	void OnCloseChild(int result);

protected:
	virtual void OnRawChar(int c);
	virtual bool OnFocus(bool focus);
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
