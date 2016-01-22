#include "Desktop.h"
#include "Editor.h"
#include "GetFileName.h"
#include "gui.h"
#include "MainMenu.h"
#include "Network.h"

#include <as/AppCfg.h>
#include <fs/FileSystem.h>
#include <loc/Language.h>
#include <ui/ConsoleBuffer.h>
#include <ui/GuiManager.h>
#include <ui/Button.h>
#include <ui/Text.h>
#include <ui/Keys.h>

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
	SetDrawBorder(true);
	SetTexture("ui/window", true);
	Resize(640, 300);

	UI::Button *button;

	const float buttonWidth = 200;
	const float buttonHeight = 128;

	float y = GetHeight() - buttonHeight;

	button = UI::Button::Create(this, _lang.single_player_btn.Get(), 0, y);
	button->SetIcon("ui/play");
	button->Resize(buttonWidth, buttonHeight);
	button->eventClick = _commands.newDM;

	button = UI::Button::Create(this, _lang.editor_btn.Get(), (GetWidth() - buttonWidth) / 2, y);
	button->SetIcon("ui/editor");
	button->Resize(buttonWidth, buttonHeight);
	button->eventClick = std::bind(&MainMenuDlg::OnEditor, this);

	button = UI::Button::Create(this, _lang.settings_btn.Get(), GetWidth() - buttonWidth, y);
	button->SetIcon("ui/settings");
	button->Resize(buttonWidth, buttonHeight);
	button->eventClick = std::bind(&MainMenuDlg::OnSettings, this);

	button = UI::Button::Create(this, _lang.return_to_game_btn.Get(), (GetWidth() - buttonWidth) / 2, 0);
	button->Resize(buttonWidth, buttonHeight);
	button->eventClick = _commands.close;

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
	_commands.gameSettings();
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
	case PT_EDITOR:
		_panelTitle->SetText(_lang.editor_title.Get());
		UI::Button::Create(_panel, _lang.editor_new_map.Get(), 0, y)->eventClick = _commands.newMap;
		UI::Button::Create(_panel, _lang.editor_load_map.Get(), 100, y)->eventClick = std::bind(&MainMenuDlg::OnImportMap, this);
		btn = UI::Button::Create(_panel, _lang.editor_save_map.Get(), 200, y);
		btn->eventClick = std::bind(&MainMenuDlg::OnExportMap, this);
		btn = UI::Button::Create(_panel, _lang.editor_map_settings.Get(), 300, y);
		btn->eventClick = std::bind(&MainMenuDlg::OnMapSettings, this);
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

