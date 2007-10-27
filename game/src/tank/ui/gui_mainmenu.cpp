// gui_mainmenu.cpp

#include "stdafx.h"

#include "gui_mainmenu.h"
#include "gui_desktop.h"
#include "gui_network.h"
#include "gui_settings.h"
#include "gui.h"

#include "GuiManager.h"

#include "Button.h"

#include "fs/FileSystem.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

MainMenuDlg::MainMenuDlg(Window *parent)
  : Dialog(parent, 0, 0, 1, 1, false)
  , _panel(NULL)
  , _ptype(PT_NONE)
  , _pstate(PS_NONE)
{
	SetBorder(false);
	SetTexture("gui_splash");
	Resize(GetTextureWidth(), GetTextureHeight());
	OnParentSize(parent->GetWidth(), parent->GetHeight());

	(new Button(this, 0, GetHeight(), "Одиночная"))->eventClick.bind(&MainMenuDlg::OnSinglePlayer, this);
	(new Button(this, 100, GetHeight(), "Сетевая"))->eventClick.bind(&MainMenuDlg::OnMultiPlayer, this);
	(new Button(this, 200, GetHeight(), "Редактор"))->eventClick.bind(&MainMenuDlg::OnEditor, this);
	(new Button(this, 300, GetHeight(), "Опции (F12)"))->eventClick.bind(&MainMenuDlg::OnSettings, this);
	(new Button(this, 416, GetHeight(), "Выход (Alt+А4)"))->eventClick.bind(&MainMenuDlg::OnExit, this);

	_panel = new Window(this, 0, GetHeight() + 40, NULL);



/*
	std::set<string_t> c;
	g_fs->GetFileSystem("campaign")->EnumAllFiles(c, "*.lua");

	float y = GetHeight() + 30;
	for( std::set<string_t>::iterator it = c.begin(); it != c.end(); ++it )
	{
		it->erase(it->length() - 4); // cut out the file extension

		DelegateAdapter1<string_t> d(*it);
		d.eventOnEvent.bind(&MainMenuDlg::OnCampaign, this);
		_campaigns.push_back(d);

		Button *btn = new Button(this, 0, y, it->c_str());
		btn->eventClick.bind(&DelegateAdapter1<string_t>::OnEvent, &_campaigns.back());

		y += btn->GetHeight() + 1;
	}
*/
}

void MainMenuDlg::OnSinglePlayer()
{
	if( ClearPanel(PT_SINGLEPLAYER) )
	{
		(new Button(_panel, 0, 0, "Кампания"));//->eventClick.bind(&MainMenuDlg::OnNewGame, this);
		(new Button(_panel, 0, 0, "Мясо (F2)"))->eventClick.bind(&MainMenuDlg::OnNewGame, this);
		new Button(_panel, 100, 0, "Загрузить");
		new Button(_panel, 200, 0, "Сохранить");
	}
}

void MainMenuDlg::OnNewGame()
{
	Show(false);
	NewGameDlg *dlg = new NewGameDlg(GetParent());
	dlg->eventClose.bind(&MainMenuDlg::OnCloseChild, this);
}

void MainMenuDlg::OnCampaign(string_t name)
{
	Close(_resultOK);
	if( !script_exec_file(g_env.L, ("campaign/" + name + ".lua").c_str()) )
	{
		static_cast<Desktop*>(g_gui->GetDesktop())->ShowConsole(true);
	}
}

void MainMenuDlg::OnMultiPlayer()
{
	if( ClearPanel(PT_MULTIPLAYER) )
	{
		(new Button(_panel, 0, 0, "Создать"))->eventClick.bind(&MainMenuDlg::OnHost, this);
		(new Button(_panel, 100, 0, "Подключиться"))->eventClick.bind(&MainMenuDlg::OnJoin, this);
	}
}

void MainMenuDlg::OnHost()
{
	Show(false);
	CreateServerDlg *dlg = new CreateServerDlg(GetParent());
	dlg->eventClose.bind(&MainMenuDlg::OnCloseChild, this);
}

void MainMenuDlg::OnJoin()
{
	Show(false);
	ConnectDlg *dlg = new ConnectDlg(GetParent());
	dlg->eventClose.bind(&MainMenuDlg::OnCloseChild, this);
}

void MainMenuDlg::OnEditor()
{
	if( ClearPanel(PT_EDITOR) )
	{
		(new Button(_panel, 0, 0, "Создать"));//->eventClick.bind(&MainMenuDlg::OnNewGame, this);
		new Button(_panel, 100, 0, "Загрузить");
		new Button(_panel, 200, 0, "Сохранить");
	}
}

void MainMenuDlg::OnExit()
{
	DestroyWindow(g_env.hMainWnd);
}

void MainMenuDlg::OnSettings()
{
	SettingsDlg *dlg = new SettingsDlg(this);
//	dlg->eventClose.bind(&MainMenuDlg::OnCloseChild, this);
}

void MainMenuDlg::OnParentSize(float width, float height)
{
	Move( (width - GetWidth()) * 0.5f, (height - GetHeight()) * 0.5f - 64 );
}

void MainMenuDlg::OnCloseChild(int result)
{
	if( Dialog::_resultOK == result )
	{
		Close(result);
	}
	else
	{
		Show(true);
		GetManager()->SetFocusWnd(this);
	}
}

void MainMenuDlg::OnRawChar(int c)
{
	switch(c)
	{
	case VK_F2:
		OnNewGame();
		break;
	case VK_F12:
		OnSettings();
		break;
	default:
		Dialog::OnRawChar(c);
	}
}

bool MainMenuDlg::ClearPanel(PanelType newtype)
{
	while( _panel->GetFirstChild() )
	{
		_panel->GetFirstChild()->Destroy();
	}
	if( _ptype != newtype )
	{
		_ptype = newtype;
		return true;
	}
	_ptype = PT_NONE;
	return false;
}


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

