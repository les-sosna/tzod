// gui_desktop.h

#pragma once

#include "Window.h"

namespace UI
{
	class Console;

///////////////////////////////////////////////////////////////////////////////

class Desktop : public Window
{
	Console *_con;

public:
	Desktop(GuiManager* manager);

	void ShowDesktopBackground(bool show);
	
	void OnCloseChild(int result);

protected:
	virtual void OnRawChar(int c);
	virtual bool OnFocus(bool focus);
	virtual void OnSize(float width, float height);
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
