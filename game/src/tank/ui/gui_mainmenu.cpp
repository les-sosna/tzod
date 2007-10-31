// gui_mainmenu.cpp

#include "stdafx.h"

#include "gui_mainmenu.h"
#include "gui_desktop.h"
#include "gui_network.h"
#include "gui_settings.h"
#include "gui_editor.h"
#include "gui_getfilename.h"
#include "gui.h"

#include "GuiManager.h"

#include "Button.h"
#include "Text.h"

#include "fs/FileSystem.h"

#include "core/Console.h"

#include "config/Config.h"

#include "network/TankClient.h"
#include "network/TankServer.h"

#include "Level.h"
#include "Macros.h"


namespace UI
{
///////////////////////////////////////////////////////////////////////////////

MainMenuDlg::MainMenuDlg(Window *parent)
  : Dialog(parent, 1, 1)
  , _panel(NULL)
  , _ptype(PT_NONE)
  , _pstate(PS_NONE)
  , _fileDlg(NULL)
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

	_panelFrame = new Window(this, 0, GetHeight() + 40, NULL);
	_panelFrame->ClipChildren(true);
	_panelFrame->Resize(GetWidth(), 64);

	_panel = new Window(_panelFrame, 0, -_panelFrame->GetHeight(), NULL);
	_panelTitle = NULL;

	if( g_level && GT_EDITOR == g_level->_gameType )
	{
		SwitchPanel(PT_EDITOR);
	}

	if( g_level && GT_DEATHMATCH == g_level->_gameType )
	{
		SwitchPanel(PT_SINGLEPLAYER);
	}


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
	SwitchPanel(PT_SINGLEPLAYER);
}

void MainMenuDlg::OnNewGame()
{
	Show(false);
	NewGameDlg *dlg = new NewGameDlg(GetParent());
	dlg->eventClose.bind(&MainMenuDlg::OnCloseChild, this);
}

void MainMenuDlg::OnCampaign()
{
	string_t name;
	Close(_resultOK);
	if( !script_exec_file(g_env.L, ("campaign/" + name + ".lua").c_str()) )
	{
		static_cast<Desktop*>(g_gui->GetDesktop())->ShowConsole(true);
	}
}

void MainMenuDlg::OnSaveGame()
{
	Show(false);

	GetFileNameDlg::Params param;
	param.title = "Сохраненить игру";
	param.folder = g_fs->GetFileSystem(DIR_SAVE);
	param.extension = "sav";

	_ASSERT(NULL == _fileDlg);
	_fileDlg = new GetFileNameDlg(GetParent(), param);
	_fileDlg->eventClose.bind(&MainMenuDlg::OnSaveGameSelect, this);
}

void MainMenuDlg::OnSaveGameSelect(int result)
{
	_ASSERT(_fileDlg);
	_ASSERT(g_level);
	if( _resultOK == result )
	{
		string_t tmp = DIR_SAVE;
		tmp += "/";
		tmp += _fileDlg->GetFileName();

		if( g_level->Serialize(tmp.c_str()) )
		{
			g_console->printf("game saved: '%s'\n", tmp.c_str());
		}
		else
		{
			g_console->printf("couldn't save game to '%s'", tmp.c_str());
			static_cast<Desktop*>(g_gui->GetDesktop())->ShowConsole(true);
		}
	}
	_fileDlg = NULL;
	OnCloseChild(result);
}

void MainMenuDlg::OnLoadGame()
{
	Show(false);

	GetFileNameDlg::Params param;
	param.title = "Загрузить игру";
	param.folder = g_fs->GetFileSystem(DIR_SAVE);
	param.extension = "sav";

	_ASSERT(NULL == _fileDlg);
	_fileDlg = new GetFileNameDlg(GetParent(), param);
	_fileDlg->eventClose.bind(&MainMenuDlg::OnLoadGameSelect, this);
}

void MainMenuDlg::OnLoadGameSelect(int result)
{
	_ASSERT(_fileDlg);
	if( _resultOK == result )
	{
		script_exec(g_env.L, "reset()");

		string_t tmp = DIR_SAVE;
		tmp += "/";
		tmp += _fileDlg->GetFileName();

		SAFE_DELETE(g_level);
		SAFE_DELETE(g_client);
		SAFE_DELETE(g_server);

		g_level = new Level();
		if( !g_level->init_load(tmp.c_str()) )
		{
			SAFE_DELETE(g_level);
			g_console->printf("couldn't load game from '%s'", tmp.c_str());
			static_cast<Desktop*>(g_gui->GetDesktop())->ShowConsole(true);
		}
	}
	_fileDlg = NULL;
	OnCloseChild(result);
}

void MainMenuDlg::OnMultiPlayer()
{
	SwitchPanel(PT_MULTIPLAYER);
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
	ConnectDlg *dlg = new ConnectDlg(GetParent(), NULL);
	dlg->eventClose.bind(&MainMenuDlg::OnCloseChild, this);
}

void MainMenuDlg::OnNetworkProfile()
{
//	Show(false);
	EditPlayerDlg *dlg = new EditPlayerDlg(GetParent(), g_conf.cl_playerinfo);
//	dlg->eventClose.bind(&MainMenuDlg::OnCloseChild, this);
}

void MainMenuDlg::OnEditor()
{
	SwitchPanel(PT_EDITOR);
}

void MainMenuDlg::OnNewMap()
{
	Show(false);
	NewMapDlg *dlg = new NewMapDlg(GetParent());
	dlg->eventClose.bind(&MainMenuDlg::OnCloseChild, this);
}

void MainMenuDlg::OnMapSettings()
{
	Show(false);
	MapSettingsDlg *dlg = new MapSettingsDlg(GetParent());
	dlg->eventClose.bind(&MainMenuDlg::OnCloseChild, this);
}

void MainMenuDlg::OnImportMap()
{
	Show(false);

	GetFileNameDlg::Params param;
	param.title = "Выбор карты для редактирования";
	param.folder = g_fs->GetFileSystem(DIR_MAPS);
	param.extension = "map";

	_ASSERT(NULL == _fileDlg);
	_fileDlg = new GetFileNameDlg(GetParent(), param);
	_fileDlg->eventClose.bind(&MainMenuDlg::OnImportMapSelect, this);
}

void MainMenuDlg::OnImportMapSelect(int result)
{
	_ASSERT(_fileDlg);
	if( _resultOK == result )
	{
		script_exec(g_env.L, "reset()");

		string_t tmp = DIR_MAPS;
		tmp += "/";
		tmp += _fileDlg->GetFileName();

		SAFE_DELETE(g_level);
		SAFE_DELETE(g_client);
		SAFE_DELETE(g_server);

		g_level = new Level();
		if( !g_level->init_import_and_edit(tmp.c_str()) )
		{
			SAFE_DELETE(g_level);
			g_console->printf("couldn't import map '%s'", tmp.c_str());
			static_cast<Desktop*>(g_gui->GetDesktop())->ShowConsole(true);
		}
	}
	_fileDlg = NULL;
	OnCloseChild(result);
}

void MainMenuDlg::OnExportMap()
{
	Show(false);

	GetFileNameDlg::Params param;
	param.title = "Сохранение карты";
	param.folder = g_fs->GetFileSystem(DIR_MAPS);
	param.extension = "map";

	_ASSERT(NULL == _fileDlg);
	_fileDlg = new GetFileNameDlg(GetParent(), param);
	_fileDlg->eventClose.bind(&MainMenuDlg::OnExportMapSelect, this);
}

void MainMenuDlg::OnExportMapSelect(int result)
{
	_ASSERT(_fileDlg);
	_ASSERT(g_level);
	if( _resultOK == result )
	{
		string_t tmp = DIR_MAPS;
		tmp += "/";
		tmp += _fileDlg->GetFileName();

		if( g_level->Export(tmp.c_str()) )
		{
			g_console->printf("map exported: '%s'\n", tmp.c_str());
			g_conf.cl_map->Set(_fileDlg->GetFileTitle().c_str());
		}
		else
		{
			g_console->printf("couldn't export map to '%s'", tmp.c_str());
			static_cast<Desktop*>(g_gui->GetDesktop())->ShowConsole(true);
		}
	}
	_fileDlg = NULL;
	OnCloseChild(result);
}

void MainMenuDlg::OnSettings()
{
	Show(false);
	SettingsDlg *dlg = new SettingsDlg(GetParent());
	dlg->eventClose.bind(&MainMenuDlg::OnCloseChild, this);
}

void MainMenuDlg::OnExit()
{
	DestroyWindow(g_env.hMainWnd);
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

void MainMenuDlg::SwitchPanel(PanelType newtype)
{
	if( _ptype != newtype )
	{
		_ptype = newtype;
	}
	else
	{
		_ptype = PT_NONE;
	}
	_pstate = PS_DISAPPEARING;
	SetTimeStep(true);
}

void MainMenuDlg::CreatePanel()
{
	_panelTitle = new Text(_panel, 0, 0, "", alignTextLT);
	_panelTitle->SetTexture("font_default");
	_panelTitle->Resize(_panelTitle->GetTextureWidth(), _panelTitle->GetTextureHeight());

	float y = _panelTitle->GetHeight() + _panelTitle->GetY() + 10;
	Button *btn;

	switch( _ptype )
	{
	case PT_SINGLEPLAYER:
		_panelTitle->SetText("Одиночная игра");
		(new Button(_panel, 0, y, "Кампания"))->eventClick.bind(&MainMenuDlg::OnCampaign, this);
		(new Button(_panel, 100, y, "Мясо (F2)"))->eventClick.bind(&MainMenuDlg::OnNewGame, this);
		(new Button(_panel, 200, y, "Загрузить"))->eventClick.bind(&MainMenuDlg::OnLoadGame, this);
		btn = new Button(_panel, 300, y, "Сохранить");
		btn->eventClick.bind(&MainMenuDlg::OnSaveGame, this);
		btn->Enable(g_level && GT_DEATHMATCH == g_level->_gameType);
		break;
	case PT_MULTIPLAYER:
		_panelTitle->SetText("Сетевая игра");
		(new Button(_panel, 0, y, "Создать"))->eventClick.bind(&MainMenuDlg::OnHost, this);
		(new Button(_panel, 100, y, "Подключиться"))->eventClick.bind(&MainMenuDlg::OnJoin, this);
		(new Button(_panel, 200, y, "Профиль"))->eventClick.bind(&MainMenuDlg::OnNetworkProfile, this);
		break;
	case PT_EDITOR:
		_panelTitle->SetText("Редактор карт");
		(new Button(_panel, 0, y, "Новая карта"))->eventClick.bind(&MainMenuDlg::OnNewMap, this);
		(new Button(_panel, 100, y, "Загрузить"))->eventClick.bind(&MainMenuDlg::OnImportMap, this);
		btn = new Button(_panel, 200, y, "Сохранить");
		btn->eventClick.bind(&MainMenuDlg::OnExportMap, this);
		btn->Enable(g_level && GT_EDITOR == g_level->_gameType);
		btn = new Button(_panel, 300, y, "Настройки");
		btn->eventClick.bind(&MainMenuDlg::OnMapSettings, this);
		btn->Enable(g_level && GT_EDITOR == g_level->_gameType);
		break;
	default:
		_ASSERT(FALSE);
	}
}

void MainMenuDlg::OnTimeStep(float dt)
{
	switch( _pstate )
	{
	case PS_APPEARING:
		_panel->Move(0, _panel->GetY() + dt * 1000);
		if( _panel->GetY() >= 0 )
		{
			_panel->Move(0, 0);
			_pstate = PS_NONE;
			SetTimeStep(false);
		}
		break;
	case PS_DISAPPEARING:
		_panel->Move(0, _panel->GetY() - dt * 1000);
		if( _panel->GetY() <= -_panelFrame->GetHeight() )
		{
			while( _panel->GetFirstChild() )
			{
				_panel->GetFirstChild()->Destroy();
			}
			_panel->Move(0, -_panelFrame->GetHeight());
			_panelTitle = NULL;

			if( PT_NONE != _ptype )
			{
				_pstate = PS_APPEARING;
				CreatePanel();
			}
			else
			{
				_pstate = PS_NONE;
				SetTimeStep(false);
			}
		}
		break;
	default:
		_ASSERT(FALSE);
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

