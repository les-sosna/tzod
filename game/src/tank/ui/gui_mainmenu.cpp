// gui_mainmenu.cpp

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

//#include "network/TankClient.h"
//#include "network/TankServer.h"

#include "Level.h"
#include "Macros.h"
#include "script.h"
#include "functions.h"

#include "fs/FileSystem.h"
#include "core/debug.h"
#include "config/Config.h"
#include "config/Language.h"


#include <GLFW/glfw3.h>


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

	SetDrawBorder(false);
	SetTexture("gui_splash", true);
	OnParentSize(parent->GetWidth(), parent->GetHeight());

	Button::Create(this, g_lang.single_player_btn.Get(), 0, GetHeight())->eventClick = std::bind(&MainMenuDlg::OnSinglePlayer, this);
	Button::Create(this, g_lang.network_btn.Get(), 100, GetHeight())->eventClick = std::bind(&MainMenuDlg::OnMultiPlayer, this);
	Button::Create(this, g_lang.editor_btn.Get(), 200, GetHeight())->eventClick = std::bind(&MainMenuDlg::OnEditor, this);
	Button::Create(this, g_lang.settings_btn.Get(), 300, GetHeight())->eventClick = std::bind(&MainMenuDlg::OnSettings, this);
	Button::Create(this, g_lang.exit_game_btn.Get(), 416, GetHeight())->eventClick = std::bind(&MainMenuDlg::OnExit, this);

	_panelFrame = Window::Create(this);
	_panelFrame->SetDrawBackground(false);
	_panelFrame->SetDrawBorder(false);
	_panelFrame->SetClipChildren(true);
	_panelFrame->Move(0, GetHeight() + 40);
	_panelFrame->Resize(GetWidth(), 64);

	_panel = Window::Create(_panelFrame);
	_panel->SetDrawBackground(false);
	_panel->SetDrawBorder(false);
	_panel->Move(0, -_panelFrame->GetHeight());
	_panelTitle = NULL;
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
	SetVisible(false);
	NewGameDlg *dlg = new NewGameDlg(GetParent(), g_level.get());
	dlg->eventClose = std::bind(&MainMenuDlg::OnCloseChild, this, std::placeholders::_1);
}

void MainMenuDlg::OnCampaign()
{
	SetVisible(false);
	NewCampaignDlg *dlg = new NewCampaignDlg(GetParent());
	dlg->eventClose = std::bind(&MainMenuDlg::OnCloseChild, this, std::placeholders::_1);
}

void MainMenuDlg::OnSaveGame()
{
	GetFileNameDlg::Params param;
	param.title = g_lang.get_file_name_save_game.Get();
	param.folder = g_fs->GetFileSystem(DIR_SAVE, true);
	param.extension = "sav";

	if( !param.folder )
	{
		static_cast<Desktop *>(GetManager()->GetDesktop())->ShowConsole(true);
		GetConsole().Printf(1, "Could not open directory '%s'", DIR_SAVE);
		return;
	}

	SetVisible(false);
	assert(NULL == _fileDlg);
	_fileDlg = new GetFileNameDlg(GetParent(), param);
	_fileDlg->eventClose = std::bind(&MainMenuDlg::OnSaveGameSelect, this, std::placeholders::_1);
}

void MainMenuDlg::OnSaveGameSelect(int result)
{
	assert(_fileDlg);
	if( _resultOK == result )
	{
		std::string tmp = DIR_SAVE;
		tmp += "/";
		tmp += _fileDlg->GetFileName();
		try
		{
			g_level->Serialize(tmp.c_str());
			GetConsole().Printf(0, "game saved: '%s'", tmp.c_str());
		}
		catch( const std::exception &e )
		{
			GetConsole().Printf(1, "Couldn't save game to '%s' - ", tmp.c_str(), e.what());
			static_cast<Desktop*>(GetManager()->GetDesktop())->ShowConsole(true);
		}
	}
	_fileDlg = NULL;
	OnCloseChild(result);
}

void MainMenuDlg::OnLoadGame()
{
	GetFileNameDlg::Params param;
	param.title = g_lang.get_file_name_load_game.Get();
	param.folder = g_fs->GetFileSystem(DIR_SAVE, false, true);
	param.extension = "sav";

	SetVisible(false);
	assert(NULL == _fileDlg);
	_fileDlg = new GetFileNameDlg(GetParent(), param);
	_fileDlg->eventClose = std::bind(&MainMenuDlg::OnLoadGameSelect, this, std::placeholders::_1);
}

void MainMenuDlg::OnLoadGameSelect(int result)
{
	assert(_fileDlg);
	if( _resultOK == result )
	{
//		SAFE_DELETE(g_client);

		std::string tmp = DIR_SAVE;
		tmp += "/";
		tmp += _fileDlg->GetFileName();

		try
		{
			g_level->Unserialize(tmp.c_str());
		}
		catch( const std::exception &e )
		{
			GetConsole().Printf(1, "Couldn't load game from '%s' - %s", tmp.c_str(), e.what());
			static_cast<Desktop*>(GetManager()->GetDesktop())->ShowConsole(true);
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
	SetVisible(false);
	CreateServerDlg *dlg = new CreateServerDlg(GetParent());
	dlg->eventClose = std::bind(&MainMenuDlg::OnCloseChild, this, std::placeholders::_1);
}

void MainMenuDlg::OnJoin()
{
	SetVisible(false);
	ConnectDlg *dlg = new ConnectDlg(GetParent(), g_conf.cl_server.Get());
	dlg->eventClose = std::bind(&MainMenuDlg::OnCloseChild, this, std::placeholders::_1);
}

void MainMenuDlg::OnInternet()
{
	SetVisible(false);
	InternetDlg *dlg = new InternetDlg(GetParent());
	dlg->eventClose = std::bind(&MainMenuDlg::OnCloseChild, this, std::placeholders::_1);
}

void MainMenuDlg::OnNetworkProfile()
{
//	SetVisible(false);
//	EditPlayerDlg *dlg =
    new EditPlayerDlg(GetParent(), g_conf.cl_playerinfo->GetRoot());
//	dlg->eventClose.bind(&MainMenuDlg::OnCloseChild, this);
}

void MainMenuDlg::OnEditor()
{
	SwitchPanel(PT_EDITOR);
}

void MainMenuDlg::OnNewMap()
{
	SetVisible(false);
	NewMapDlg *dlg = new NewMapDlg(GetParent());
	dlg->eventClose = std::bind(&MainMenuDlg::OnCloseChild, this, std::placeholders::_1);
}

void MainMenuDlg::OnMapSettings()
{
	SetVisible(false);
	MapSettingsDlg *dlg = new MapSettingsDlg(GetParent());
	dlg->eventClose = std::bind(&MainMenuDlg::OnCloseChild, this, std::placeholders::_1);
}

void MainMenuDlg::OnImportMap()
{
	GetFileNameDlg::Params param;
	param.title = g_lang.get_file_name_load_map.Get();
	param.folder = g_fs->GetFileSystem(DIR_MAPS);
	param.extension = "map";

	if( !param.folder )
	{
		static_cast<Desktop *>(GetManager()->GetDesktop())->ShowConsole(true);
		TRACE("ERROR: Could not open directory '%s'", DIR_MAPS);
		return;
	}

	SetVisible(false);
	assert(NULL == _fileDlg);
	_fileDlg = new GetFileNameDlg(GetParent(), param);
	_fileDlg->eventClose = std::bind(&MainMenuDlg::OnImportMapSelect, this, std::placeholders::_1);
}

void MainMenuDlg::OnImportMapSelect(int result)
{
	assert(_fileDlg);
	if( _resultOK == result )
	{
//		SAFE_DELETE(g_client);

		std::string tmp = DIR_MAPS;
		tmp += "/";
		tmp += _fileDlg->GetFileName();

		if( !g_level->init_import_and_edit(tmp.c_str()) )
		{
			GetConsole().Printf(1, "couldn't import map '%s'", tmp.c_str());
			static_cast<Desktop*>(GetManager()->GetDesktop())->ShowConsole(true);
		}
	}
	_fileDlg = NULL;
	OnCloseChild(result);
}

void MainMenuDlg::OnExportMap()
{
	GetFileNameDlg::Params param;
	param.title = g_lang.get_file_name_save_map.Get();
	param.folder = g_fs->GetFileSystem(DIR_MAPS, true);
	param.extension = "map";

	if( !param.folder )
	{
		static_cast<Desktop *>(GetManager()->GetDesktop())->ShowConsole(true);
		TRACE("ERROR: Could not open directory '%s'", DIR_MAPS);
		return;
	}

	SetVisible(false);
	assert(NULL == _fileDlg);
	_fileDlg = new GetFileNameDlg(GetParent(), param);
	_fileDlg->eventClose = std::bind(&MainMenuDlg::OnExportMapSelect, this, std::placeholders::_1);
}

void MainMenuDlg::OnExportMapSelect(int result)
{
	assert(_fileDlg);
	if( _resultOK == result )
	{
		std::string tmp = DIR_MAPS;
		tmp += "/";
		tmp += _fileDlg->GetFileName();

		try
		{
			g_level->Export(g_fs->Open(tmp, FS::ModeWrite)->QueryStream());
		}
		catch( const std::exception &e )
		{
			GetConsole().Printf(1, "Couldn't export map to '%s' - ", tmp.c_str(), e.what());
			static_cast<Desktop*>(GetManager()->GetDesktop())->ShowConsole(true);
		}

		GetConsole().Printf(0, "map exported: '%s'", tmp.c_str());
		g_conf.cl_map.Set(_fileDlg->GetFileTitle());
	}
	_fileDlg = NULL;
	OnCloseChild(result);
}

void MainMenuDlg::OnSettings()
{
	SetVisible(false);
	SettingsDlg *dlg = new SettingsDlg(GetParent());
	dlg->eventClose = std::bind(&MainMenuDlg::OnCloseChild, this, std::placeholders::_1);
}

void MainMenuDlg::OnExit()
{
    glfwSetWindowShouldClose(g_appWindow, 1);
}

void MainMenuDlg::OnParentSize(float width, float height)
{
	Move( (width - GetWidth()) * 0.5f, (height - GetHeight()) * 0.5f );
}

void MainMenuDlg::OnCloseChild(int result)
{
	if( Dialog::_resultOK == result )
	{
		Close(result);
	}
	else
	{
		SetVisible(true);
		GetManager()->SetFocusWnd(this);
	}
}

bool MainMenuDlg::OnRawChar(int c)
{
	switch(c)
	{
	case GLFW_KEY_F2:
		OnNewGame();
		break;
	case GLFW_KEY_F12:
		OnSettings();
		break;
	default:
		return Dialog::OnRawChar(c);
	}
	return true;
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
	_panelTitle = Text::Create(_panel, 0, 0, "", alignTextLT);
	_panelTitle->SetFont("font_default");

	float y = _panelTitle->GetCharHeight() + _panelTitle->GetY() + 10;
	Button *btn;

	switch( _ptype )
	{
	case PT_SINGLEPLAYER:
		_panelTitle->SetText(g_lang.single_player_title.Get());
		Button::Create(_panel, g_lang.single_player_campaign.Get(), 0, y)->eventClick = std::bind(&MainMenuDlg::OnCampaign, this);
		Button::Create(_panel, g_lang.single_player_skirmish.Get(), 100, y)->eventClick = std::bind(&MainMenuDlg::OnNewGame, this);
		Button::Create(_panel, g_lang.single_player_load.Get(), 200, y)->eventClick = std::bind(&MainMenuDlg::OnLoadGame, this);
		btn = Button::Create(_panel, g_lang.single_player_save.Get(), 300, y);
		btn->eventClick = std::bind(&MainMenuDlg::OnSaveGame, this);
//		btn->SetEnabled(g_client && g_client->SupportSave());
		break;
	case PT_MULTIPLAYER:
		_panelTitle->SetText(g_lang.network_title.Get());
		Button::Create(_panel, g_lang.network_host.Get(), 0, y)->eventClick = std::bind(&MainMenuDlg::OnHost, this);
		Button::Create(_panel, g_lang.network_join.Get(), 100, y)->eventClick = std::bind(&MainMenuDlg::OnJoin, this);
		Button::Create(_panel, g_lang.network_internet.Get(), 200, y)->eventClick = std::bind(&MainMenuDlg::OnInternet, this);
		Button::Create(_panel, g_lang.network_profile.Get(), 300, y)->eventClick = std::bind(&MainMenuDlg::OnNetworkProfile, this);
		break;
	case PT_EDITOR:
		_panelTitle->SetText(g_lang.editor_title.Get());
		Button::Create(_panel, g_lang.editor_new_map.Get(), 0, y)->eventClick = std::bind(&MainMenuDlg::OnNewMap, this);
		Button::Create(_panel, g_lang.editor_load_map.Get(), 100, y)->eventClick = std::bind(&MainMenuDlg::OnImportMap, this);
		btn = Button::Create(_panel, g_lang.editor_save_map.Get(), 200, y);
		btn->eventClick = std::bind(&MainMenuDlg::OnExportMap, this);
//		btn->SetEnabled(g_client && g_client->SupportEditor());
		btn = Button::Create(_panel, g_lang.editor_map_settings.Get(), 300, y);
		btn->eventClick = std::bind(&MainMenuDlg::OnMapSettings, this);
//		btn->SetEnabled(g_client && g_client->SupportEditor());
		break;
	default:
		assert(false);
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
		assert(false);
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

