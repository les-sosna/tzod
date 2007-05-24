// gui_desktop.cpp

#include "stdafx.h"

#include "gui_widgets.h"
#include "gui_desktop.h"
#include "gui_console.h"
#include "gui.h"

#include "GuiManager.h"

#include "config/Config.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

Desktop::Desktop(GuiManager* manager) : Window(manager)
{
	_con = new Console(this, 10, 0);
	_con->Show(false);

	_fps = new FpsCounter(this, 0, 0, alignTextLB);
	g_conf.ui_showfps->eventChange.bind( &Desktop::OnChangeShowFps, this );
	OnChangeShowFps();

	_time = new TimeElapsed(this, 0, 0, alignTextRB );
	g_conf.ui_showtime->eventChange.bind( &Desktop::OnChangeShowTime, this );
	OnChangeShowTime();

	OnRawChar(VK_ESCAPE); // to invoke main menu dialog
}

Desktop::~Desktop()
{
	g_conf.ui_showfps->eventChange.clear();
	g_conf.ui_showtime->eventChange.clear();
}

void Desktop::ShowDesktopBackground(bool show)
{
	SetTexture(show ? "window" : NULL);
}

void Desktop::OnCloseChild(int result)
{
	ShowDesktopBackground(false);
}

void Desktop::OnRawChar(int c)
{
	Dialog *dlg = NULL;

	switch( c )
	{
	case VK_OEM_3: // '~'
		_con->Show(true);
		break;

	case VK_ESCAPE:
		dlg = new MainMenuDlg(this);
		ShowDesktopBackground(true);
		dlg->eventClose.bind( &Desktop::OnCloseChild, this );
		break;

	case VK_F2:
		dlg = new NewGameDlg(this);
		ShowDesktopBackground(true);
		dlg->eventClose.bind( &Desktop::OnCloseChild, this );
		break;
	}
}

bool Desktop::OnFocus(bool focus)
{
	return true;
}

void Desktop::OnSize(float width, float height)
{
	_con->Resize(GetWidth() - 20, GetHeight() * 0.5f);
	_fps->Move(1, GetHeight() - 1);
	_time->Move( GetWidth() - 1, GetHeight() - 1 );
}

void Desktop::OnChangeShowFps()
{
	_fps->Show(g_conf.ui_showfps->Get());
}

void Desktop::OnChangeShowTime()
{
	_fps->Show(g_conf.ui_showfps->Get());
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
