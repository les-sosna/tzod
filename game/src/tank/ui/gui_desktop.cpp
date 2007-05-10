// gui_desktop.cpp

#include "stdafx.h"
#include "gui_desktop.h"
#include "gui.h"

#include "GuiManager.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

Desktop::Desktop(GuiManager* manager) : Window(manager)
{
	OnRawChar(VK_ESCAPE); // to invoke main menu dialog
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
	case VK_ESCAPE:
		dlg = new MainMenuDlg(this);
		break;

	case VK_F2:
		dlg = new NewGameDlg(this);
		break;
	}

	if( dlg )
	{
		ShowDesktopBackground(true);
		dlg->eventClose.bind( &Desktop::OnCloseChild, this );
	}
}

bool Desktop::OnFocus(bool focus)
{
	return true;
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
