#include "Desktop.h"
#include "Editor.h"
#include "GetFileName.h"
#include "gui.h"
#include "MainMenu.h"
#include "Network.h"
#include "Settings.h"

#include <as/AppCfg.h>
#include <fs/FileSystem.h>
#include <loc/Language.h>
#include <ui/ConsoleBuffer.h>
#include <ui/GuiManager.h>
#include <ui/Button.h>
#include <ui/Text.h>
#include <ui/Keys.h>

static const float c_firstRowHeight = 30.f;

MainMenuDlg::MainMenuDlg(Window *parent,
                         FS::FileSystem &fs,
                         ConfCache &conf,
                         LangCache &lang,
                         UI::ConsoleBuffer &logger,
                         MainMenuCommands commands)
  : Window(parent)
  , _panel(nullptr)
  , _ptype(PT_NONE)
  , _pstate(PS_NONE)
  , _fileDlg(nullptr)
  , _fs(fs)
  , _conf(conf)
  , _lang(lang)
  , _logger(logger)
  , _commands(std::move(commands))
{
	SetDrawBorder(false);
	SetTexture("gui_splash", true);
	OnParentSize(parent->GetWidth(), parent->GetHeight());

	UI::Button::Create(this, _lang.single_player_btn.Get(), 0, GetHeight())->eventClick = std::bind(&MainMenuDlg::OnSinglePlayer, this);
	UI::Button::Create(this, _lang.network_btn.Get(), 100, GetHeight())->eventClick = std::bind(&MainMenuDlg::OnMultiPlayer, this);
	UI::Button::Create(this, _lang.editor_btn.Get(), 200, GetHeight())->eventClick = std::bind(&MainMenuDlg::OnEditor, this);
	UI::Button::Create(this, _lang.settings_btn.Get(), 300, GetHeight())->eventClick = std::bind(&MainMenuDlg::OnSettings, this);
	UI::Button::Create(this, _lang.return_to_game_btn.Get(), 416, GetHeight())->eventClick = _commands.close;

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
	_panelTitle = nullptr;
}

MainMenuDlg::~MainMenuDlg()
{
}

void MainMenuDlg::OnSinglePlayer()
{
	SwitchPanel(PT_SINGLEPLAYER);
}

void MainMenuDlg::OnSaveGame()
{
	GetFileNameDlg::Params param;
	param.title = _lang.get_file_name_save_game.Get();
	param.folder = _fs.GetFileSystem(DIR_SAVE, true);
	param.extension = "sav";

	if( !param.folder )
	{
		static_cast<Desktop *>(GetManager().GetDesktop())->ShowConsole(true);
		_logger.Printf(1, "Could not open directory '%s'", DIR_SAVE);
		return;
	}

	SetVisible(false);
	assert(nullptr == _fileDlg);
	_fileDlg = new GetFileNameDlg(GetParent(), param, _lang);
	_fileDlg->eventClose = std::bind(&MainMenuDlg::OnSaveGameSelect, this, std::placeholders::_1);
}

void MainMenuDlg::OnSaveGameSelect(int result)
{
	assert(_fileDlg);
	if(UI::Dialog::_resultOK == result )
	{
		std::string tmp = DIR_SAVE;
		tmp += "/";
		tmp += _fileDlg->GetFileName();
		try
		{
			_logger.Printf(0, "Saving game to file '%S'...", tmp.c_str());
			throw std::runtime_error("not implemented");
//			_gameContext.Serialize(*_fs.Open(tmp, FS::ModeWrite)->QueryStream());
			_logger.Printf(0, "game saved: '%s'", tmp.c_str());
		}
		catch( const std::exception &e )
		{
			_logger.Printf(1, "Couldn't save game to '%s' - ", tmp.c_str(), e.what());
			static_cast<Desktop*>(GetManager().GetDesktop())->ShowConsole(true);
		}
	}
	_fileDlg = nullptr;
	OnCloseChild(result);
}

void MainMenuDlg::OnLoadGame()
{
	GetFileNameDlg::Params param;
	param.title = _lang.get_file_name_load_game.Get();
	param.folder = _fs.GetFileSystem(DIR_SAVE, false, true);
	param.extension = "sav";

	SetVisible(false);
	assert(nullptr == _fileDlg);
	_fileDlg = new GetFileNameDlg(GetParent(), param, _lang);
	_fileDlg->eventClose = std::bind(&MainMenuDlg::OnLoadGameSelect, this, std::placeholders::_1);
}

void MainMenuDlg::OnLoadGameSelect(int result)
{
	assert(_fileDlg);
	if(UI::Dialog::_resultOK == result )
	{
		std::string tmp = DIR_SAVE;
		tmp += "/";
		tmp += _fileDlg->GetFileName();

		try
		{
			_logger.Printf(0, "Loading saved game from file '%s'...", tmp.c_str());
			std::shared_ptr<FS::Stream> stream = _fs.Open(tmp, FS::ModeRead)->QueryStream();

//			_gameContext.Deserialize(*stream);
		}
		catch( const std::exception &e )
		{
			_logger.Printf(1, "Couldn't load game from '%s' - %s", tmp.c_str(), e.what());
			static_cast<Desktop*>(GetManager().GetDesktop())->ShowConsole(true);
		}
	}
	_fileDlg = nullptr;
	OnCloseChild(result);
}

void MainMenuDlg::OnMultiPlayer()
{
	SwitchPanel(PT_MULTIPLAYER);
}

void MainMenuDlg::OnHost()
{
//	SetVisible(false);
//	CreateServerDlg *dlg = new CreateServerDlg(GetParent(), _world, _fs);
//	dlg->eventClose = std::bind(&MainMenuDlg::OnCloseChild, this, std::placeholders::_1);
}

void MainMenuDlg::OnJoin()
{
//	SetVisible(false);
//	ConnectDlg *dlg = new ConnectDlg(GetParent(), _conf.cl_server.Get(), _world);
//	dlg->eventClose = std::bind(&MainMenuDlg::OnCloseChild, this, std::placeholders::_1);
}

void MainMenuDlg::OnInternet()
{
//	SetVisible(false);
//	InternetDlg *dlg = new InternetDlg(GetParent(), _world);
//	dlg->eventClose = std::bind(&MainMenuDlg::OnCloseChild, this, std::placeholders::_1);
}

void MainMenuDlg::OnNetworkProfile()
{
//	SetVisible(false);
//	EditPlayerDlg *dlg =
    new EditPlayerDlg(GetParent(), _conf.cl_playerinfo, _conf, _lang);
//	dlg->eventClose.bind(&MainMenuDlg::OnCloseChild, this);
}

void MainMenuDlg::OnEditor()
{
	SwitchPanel(PT_EDITOR);
}

void MainMenuDlg::OnMapSettings()
{
//	SetVisible(false);
//	MapSettingsDlg *dlg = new MapSettingsDlg(GetParent(), _gameContext);
//	dlg->eventClose = std::bind(&MainMenuDlg::OnCloseChild, this, std::placeholders::_1);
}

void MainMenuDlg::OnImportMap()
{
	GetFileNameDlg::Params param;
	param.title = _lang.get_file_name_load_map.Get();
	param.folder = _fs.GetFileSystem(DIR_MAPS);
	param.extension = "map";

	if( !param.folder )
	{
		static_cast<Desktop *>(GetManager().GetDesktop())->ShowConsole(true);
		_logger.Printf(1, "Could not open directory '%s'", DIR_MAPS);
		return;
	}

	SetVisible(false);
	assert(nullptr == _fileDlg);
	_fileDlg = new GetFileNameDlg(GetParent(), param, _lang);
	_fileDlg->eventClose = std::bind(&MainMenuDlg::OnImportMapSelect, this, std::placeholders::_1);
}

void MainMenuDlg::OnImportMapSelect(int result)
{
	assert(_fileDlg);
	if(UI::Dialog::_resultOK == result )
	{
		_commands.openMap(std::string(DIR_MAPS) + "/" + _fileDlg->GetFileName());
	}
	_fileDlg = nullptr;
	OnCloseChild(result);
}

void MainMenuDlg::OnExportMap()
{
	GetFileNameDlg::Params param;
	param.title = _lang.get_file_name_save_map.Get();
	param.folder = _fs.GetFileSystem(DIR_MAPS, true);
	param.extension = "map";

	if( !param.folder )
	{
		static_cast<Desktop *>(GetManager().GetDesktop())->ShowConsole(true);
		_logger.Printf(1, "ERROR: Could not open directory '%s'", DIR_MAPS);
		return;
	}

	SetVisible(false);
	assert(nullptr == _fileDlg);
	_fileDlg = new GetFileNameDlg(GetParent(), param, _lang);
	_fileDlg->eventClose = std::bind(&MainMenuDlg::OnExportMapSelect, this, std::placeholders::_1);
}

void MainMenuDlg::OnExportMapSelect(int result)
{
	assert(_fileDlg);
	if(UI::Dialog::_resultOK == result )
	{
		_commands.exportMap(std::string(DIR_MAPS) + "/" + _fileDlg->GetFileName());
	}
	_fileDlg = nullptr;
	OnCloseChild(result);
}

void MainMenuDlg::OnSettings()
{
	SetVisible(false);
	SettingsDlg *dlg = new SettingsDlg(GetParent(), _conf, _lang);
	dlg->eventClose = std::bind(&MainMenuDlg::OnCloseChild, this, std::placeholders::_1);
}

void MainMenuDlg::OnParentSize(float width, float height)
{
	Move(std::floor((width - GetWidth()) / 2), std::floor((height - GetHeight() - c_firstRowHeight) / 2));
}

void MainMenuDlg::OnCloseChild(int result)
{
	if(UI::Dialog::_resultOK == result )
	{
//		Close(result);
	}
	else
	{
		SetVisible(true);
		GetManager().SetFocusWnd(this);
	}
}

bool MainMenuDlg::OnKeyPressed(UI::Key key)
{
	switch(key)
	{
	case UI::Key::F12:
		OnSettings();
		break;
	case UI::Key::Escape:
		if (_ptype != PT_NONE)
			SwitchPanel(PT_NONE);
		else
			return false;
		break;
	default:
		return false;
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
	_panelTitle = UI::Text::Create(_panel, 0, 0, "", alignTextLT);
	_panelTitle->SetFont("font_default");

	float y = _panelTitle->GetCharHeight() + _panelTitle->GetY() + 10;
	UI::Button *btn;

	switch( _ptype )
	{
	case PT_SINGLEPLAYER:
		_panelTitle->SetText(_lang.single_player_title.Get());
		UI::Button::Create(_panel, _lang.single_player_campaign.Get(), 0, y)->eventClick = _commands.newCampaign;
		UI::Button::Create(_panel, _lang.single_player_skirmish.Get(), 100, y)->eventClick = _commands.newDM;
		UI::Button::Create(_panel, _lang.single_player_load.Get(), 200, y)->eventClick = std::bind(&MainMenuDlg::OnLoadGame, this);
		btn = UI::Button::Create(_panel, _lang.single_player_save.Get(), 300, y);
		btn->eventClick = std::bind(&MainMenuDlg::OnSaveGame, this);
//		btn->SetEnabled(g_client && g_client->SupportSave());
		break;
	case PT_MULTIPLAYER:
		_panelTitle->SetText(_lang.network_title.Get());
		UI::Button::Create(_panel, _lang.network_host.Get(), 0, y)->eventClick = std::bind(&MainMenuDlg::OnHost, this);
		UI::Button::Create(_panel, _lang.network_join.Get(), 100, y)->eventClick = std::bind(&MainMenuDlg::OnJoin, this);
		UI::Button::Create(_panel, _lang.network_internet.Get(), 200, y)->eventClick = std::bind(&MainMenuDlg::OnInternet, this);
		UI::Button::Create(_panel, _lang.network_profile.Get(), 300, y)->eventClick = std::bind(&MainMenuDlg::OnNetworkProfile, this);
		break;
	case PT_EDITOR:
		_panelTitle->SetText(_lang.editor_title.Get());
		UI::Button::Create(_panel, _lang.editor_new_map.Get(), 0, y)->eventClick = _commands.newMap;
		UI::Button::Create(_panel, _lang.editor_load_map.Get(), 100, y)->eventClick = std::bind(&MainMenuDlg::OnImportMap, this);
		btn = UI::Button::Create(_panel, _lang.editor_save_map.Get(), 200, y);
		btn->eventClick = std::bind(&MainMenuDlg::OnExportMap, this);
//		btn->SetEnabled(g_client && g_client->SupportEditor());
		btn = UI::Button::Create(_panel, _lang.editor_map_settings.Get(), 300, y);
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
			_panelTitle = nullptr;

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

