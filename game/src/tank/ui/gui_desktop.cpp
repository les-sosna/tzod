// gui_desktop.cpp

#include "stdafx.h"

#include "gui_desktop.h"
#include "gui_console.h"
#include "gui.h"

#include "GuiManager.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

Desktop::Desktop(GuiManager* manager) : Window(manager)
{
	_con = new Console(this, 10, 0);
	_con->Show(false);

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
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
