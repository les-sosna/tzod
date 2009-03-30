// gui_mainmenu.cpp

#include "stdafx.h"

#include "gui_mainmenu.h"
#include "gui_desktop.h"
#include "gui_network.h"
#include "gui_settings.h"
#include "gui_editor.h"
#include "gui_getfilename.h"
#include "gui_campaign.h"
#include "gui.h"

#include "GuiManager.h"

#include "Button.h"
#include "Text.h"

#include "fs/FileSystem.h"

#include "core/Console.h"
#include "core/Application.h"
#include "core/debug.h"

#include "config/Config.h"
#include "config/Language.h"

#include "network/TankClient.h"
#include "network/TankServer.h"

#include "Level.h"
#include "Macros.h"
#include "script.h"
#include "functions.h"


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
	PauseGame(true);

	SetBorder(false);
	SetTexture("gui_splash");
	Resize(GetTextureWidth(), GetTextureHeight());
	OnParentSize(parent->GetWidth(), parent->GetHeight());

	(new Button(this, 0, GetHeight(), g_lang->single_player_btn->Get()))->eventClick.bind(&MainMenuDlg::OnSinglePlayer, this);
	(new Button(this, 100, GetHeight(), g_lang->network_btn->Get()))->eventClick.bind(&MainMenuDlg::OnMultiPlayer, this);
	(new Button(this, 200, GetHeight(), g_lang->editor_btn->Get()))->eventClick.bind(&MainMenuDlg::OnEditor, this);
	(new Button(this, 300, GetHeight(), g_lang->settings_btn->Get()))->eventClick.bind(&MainMenuDlg::OnSettings, this);
	(new Button(this, 416, GetHeight(), g_lang->exit_game_btn->Get()))->eventClick.bind(&MainMenuDlg::OnExit, this);

	_panelFrame = new Window(this, 0, GetHeight() + 40, NULL);
	_panelFrame->ClipChildren(true);
	_panelFrame->Resize(GetWidth(), 64);

	_panel = new Window(_panelFrame, 0, -_panelFrame->GetHeight(), NULL);
	_panelTitle = NULL;

	if( !g_level->IsEmpty() && GT_EDITOR == g_level->_gameType )
	{
		SwitchPanel(PT_EDITOR);
	}

	if( !g_level->IsEmpty() && GT_DEATHMATCH == g_level->_gameType )
	{
		SwitchPanel(PT_SINGLEPLAYER);
	}
}

MainMenuDlg::~MainMenuDlg()
{
	PauseGame(false);
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
	Show(false);
	NewCampaignDlg *dlg = new NewCampaignDlg(GetParent());
	dlg->eventClose.bind(&MainMenuDlg::OnCloseChild, this);
}

void MainMenuDlg::OnSaveGame()
{
	GetFileNameDlg::Params param;
	param.title = g_lang->get_file_name_save_game->Get();
	param.folder = g_fs->GetFileSystem(DIR_SAVE, true);
	param.extension = "sav";

	if( !param.folder )
	{
		static_cast<Desktop *>(GetManager()->GetDesktop())->ShowConsole(true);
		TRACE("ERROR: Could not open directory '%s'\n", DIR_SAVE);
		return;
	}

	Show(false);
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
			g_app->GetConsole()->printf("game saved: '%s'\n", tmp.c_str());
		}
		else
		{
			g_app->GetConsole()->printf("couldn't save game to '%s'", tmp.c_str());
			static_cast<Desktop*>(g_gui->GetDesktop())->ShowConsole(true);
		}
	}
	_fileDlg = NULL;
	OnCloseChild(result);
}

void MainMenuDlg::OnLoadGame()
{
	GetFileNameDlg::Params param;
	param.title = g_lang->get_file_name_load_game->Get();
	param.folder = g_fs->GetFileSystem(DIR_SAVE);
	param.extension = "sav";

	if( !param.folder )
	{
		static_cast<Desktop *>(GetManager()->GetDesktop())->ShowConsole(true);
		TRACE("ERROR: Could not open directory '%s'\n", DIR_SAVE);
		return;
	}

	Show(false);
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

		if( !g_level->init_load(tmp.c_str()) )
		{
			g_app->GetConsole()->printf("couldn't load game from '%s'", tmp.c_str());
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

void MainMenuDlg::OnInternet()
{
	Show(false);
	InternetDlg *dlg = new InternetDlg(GetParent());
	dlg->eventClose.bind(&MainMenuDlg::OnCloseChild, this);
}

void MainMenuDlg::OnNetworkProfile()
{
//	Show(false);
	EditPlayerDlg *dlg = new EditPlayerDlg(GetParent(), g_conf->cl_playerinfo);
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
	GetFileNameDlg::Params param;
	param.title = g_lang->get_file_name_load_map->Get();
	param.folder = g_fs->GetFileSystem(DIR_MAPS);
	param.extension = "map";

	if( !param.folder )
	{
		static_cast<Desktop *>(GetManager()->GetDesktop())->ShowConsole(true);
		TRACE("ERROR: Could not open directory '%s'\n", DIR_MAPS);
		return;
	}

	Show(false);
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

		if( !g_level->init_import_and_edit(tmp.c_str()) )
		{
			g_app->GetConsole()->printf("couldn't import map '%s'", tmp.c_str());
			static_cast<Desktop*>(g_gui->GetDesktop())->ShowConsole(true);
		}
		else
		{
			static_cast<UI::Desktop*>(g_gui->GetDesktop())->ShowEditor(true);
		}
	}
	_fileDlg = NULL;
	OnCloseChild(result);
}

void MainMenuDlg::OnExportMap()
{
	GetFileNameDlg::Params param;
	param.title = g_lang->get_file_name_save_map->Get();
	param.folder = g_fs->GetFileSystem(DIR_MAPS, true);
	param.extension = "map";

	if( !param.folder )
	{
		static_cast<Desktop *>(GetManager()->GetDesktop())->ShowConsole(true);
		TRACE("ERROR: Could not open directory '%s'\n", DIR_MAPS);
		return;
	}

	Show(false);
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
			g_app->GetConsole()->printf("map exported: '%s'\n", tmp.c_str());
			g_conf->cl_map->Set(_fileDlg->GetFileTitle());
		}
		else
		{
			g_app->GetConsole()->printf("couldn't export map to '%s'", tmp.c_str());
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
		_panelTitle->SetText(g_lang->single_player_title->Get());
		(new Button(_panel, 0, y, g_lang->single_player_campaign->Get()))->eventClick.bind(&MainMenuDlg::OnCampaign, this);
		(new Button(_panel, 100, y, g_lang->single_player_skirmish->Get()))->eventClick.bind(&MainMenuDlg::OnNewGame, this);
		(new Button(_panel, 200, y, g_lang->single_player_load->Get()))->eventClick.bind(&MainMenuDlg::OnLoadGame, this);
		btn = new Button(_panel, 300, y, g_lang->single_player_save->Get());
		btn->eventClick.bind(&MainMenuDlg::OnSaveGame, this);
		btn->Enable(!g_level->IsEmpty() && GT_DEATHMATCH == g_level->_gameType && !g_client);
		break;
	case PT_MULTIPLAYER:
		_panelTitle->SetText(g_lang->network_title->Get());
		(new Button(_panel, 0, y, g_lang->network_host->Get()))->eventClick.bind(&MainMenuDlg::OnHost, this);
		(new Button(_panel, 100, y, g_lang->network_join->Get()))->eventClick.bind(&MainMenuDlg::OnJoin, this);
		(new Button(_panel, 200, y, g_lang->network_internet->Get()))->eventClick.bind(&MainMenuDlg::OnInternet, this);
		(new Button(_panel, 300, y, g_lang->network_profile->Get()))->eventClick.bind(&MainMenuDlg::OnNetworkProfile, this);
		break;
	case PT_EDITOR:
		_panelTitle->SetText(g_lang->editor_title->Get());
		(new Button(_panel, 0, y, g_lang->editor_new_map->Get()))->eventClick.bind(&MainMenuDlg::OnNewMap, this);
		(new Button(_panel, 100, y, g_lang->editor_load_map->Get()))->eventClick.bind(&MainMenuDlg::OnImportMap, this);
		btn = new Button(_panel, 200, y, g_lang->editor_save_map->Get());
		btn->eventClick.bind(&MainMenuDlg::OnExportMap, this);
		btn->Enable(!g_level->IsEmpty() && GT_EDITOR == g_level->_gameType);
		btn = new Button(_panel, 300, y, g_lang->editor_map_settings->Get());
		btn->eventClick.bind(&MainMenuDlg::OnMapSettings, this);
		btn->Enable(!g_level->IsEmpty() && GT_EDITOR == g_level->_gameType);
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

