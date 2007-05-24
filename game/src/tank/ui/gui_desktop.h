// gui_desktop.h

#pragma once

#include "Window.h"

namespace UI
{
	class Console;

	// widgets
	class FpsCounter;
	class TimeElapsed;

///////////////////////////////////////////////////////////////////////////////

class Desktop : public Window
{
	Console      *_con;
	FpsCounter   *_fps;
	TimeElapsed  *_time;

public:
	Desktop(GuiManager* manager);
	~Desktop();

	void ShowDesktopBackground(bool show);
	
	void OnCloseChild(int result);

protected:
	virtual void OnRawChar(int c);
	virtual bool OnFocus(bool focus);
	virtual void OnSize(float width, float height);

private:
	void OnChangeShowFps();
	void OnChangeShowTime();
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
