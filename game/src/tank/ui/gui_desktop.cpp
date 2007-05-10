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
}

void Desktop::OnRawChar(int c)
{
	switch( c )
	{
	case VK_ESCAPE:
		new MainMenuDlg(this);
		break;

	case VK_F2:
		new NewGameDlg(this);
		break;
	}
}

bool Desktop::OnFocus(bool focus)
{
	return true;
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
