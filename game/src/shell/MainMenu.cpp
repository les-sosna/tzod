#include "Editor.h"
#include "gui.h"
#include "MainMenu.h"
#include "Network.h"

#include <fs/FileSystem.h>
#include <loc/Language.h>
#include <ui/ConsoleBuffer.h>
#include <ui/GuiManager.h>
#include <ui/Button.h>
#include <ui/Text.h>
#include <ui/Keys.h>

MainMenuDlg::MainMenuDlg(UI::LayoutManager &manager,
                         TextureManager &texman,
                         FS::FileSystem &fs,
                         ConfCache &conf,
                         LangCache &lang,
                         UI::ConsoleBuffer &logger,
                         MainMenuCommands commands)
  : Window(manager)
  , _ptype(PT_NONE)
  , _pstate(PS_NONE)
  , _fs(fs)
  , _lang(lang)
  , _logger(logger)
  , _commands(std::move(commands))
{
	Resize(640, 300);

	std::shared_ptr<UI::Button> button;

	const float buttonWidth = 200;
	const float buttonHeight = 128;

	float y = (GetHeight() - buttonHeight) / 2;

	button = std::make_shared<UI::Button>(manager, texman);
	button->SetText(texman, _lang.single_player_btn.Get());
	button->Move(0, y);
	button->SetIcon(manager, texman, "ui/play");
	button->Resize(buttonWidth, buttonHeight);
	button->eventClick = _commands.newDM;
	AddFront(button);

	button = std::make_shared<UI::Button>(manager, texman);
	button->SetText(texman, _lang.editor_btn.Get());
	button->Move((GetWidth() - buttonWidth) / 2, y);
	button->SetIcon(manager, texman, "ui/editor");
	button->Resize(buttonWidth, buttonHeight);
	button->eventClick = std::bind(&MainMenuDlg::OnEditor, this);
	AddFront(button);

	button = std::make_shared<UI::Button>(manager, texman);
	button->SetText(texman, _lang.settings_btn.Get());
	button->Move(GetWidth() - buttonWidth, y);
	button->SetIcon(manager, texman, "ui/settings");
	button->Resize(buttonWidth, buttonHeight);
	button->eventClick = _commands.gameSettings;
	AddFront(button);

	_panelFrame = std::make_shared<Window>(manager);
	_panelFrame->SetClipChildren(true);
	_panelFrame->Move(0, GetHeight() + 40);
	_panelFrame->Resize(GetWidth(), 64);
	AddFront(_panelFrame);

	_panel = std::make_shared<Window>(manager);
	_panel->Move(0, -_panelFrame->GetHeight());
	_panelFrame->AddFront(_panel);
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
	_commands.mapSettings();
}

bool MainMenuDlg::OnKeyPressed(UI::InputContext &ic, UI::Key key)
{
	switch(key)
	{
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

void MainMenuDlg::CreatePanel(TextureManager &texman)
{
	auto &manager = GetManager();

	_panelTitle = std::make_shared<UI::Text>(manager, texman);
	_panelTitle->SetFont(texman, "font_default");
	_panel->AddFront(_panelTitle);

	float y = _panelTitle->GetHeight() + _panelTitle->GetY() + 10;
	std::shared_ptr<UI::Button> btn;

	switch( _ptype )
	{
	case PT_EDITOR:
		_panelTitle->SetText(texman, _lang.editor_title.Get());

		btn = std::make_shared<UI::Button>(manager, texman);
		btn->SetText(texman, _lang.editor_new_map.Get());
		btn->Move(0, y);
		btn->eventClick = _commands.newMap;
		_panel->AddFront(btn);

		btn = std::make_shared<UI::Button>(manager, texman);
		btn->SetText(texman, _lang.editor_load_map.Get());
		btn->Move(100, y);
		btn->eventClick = _commands.openMap;
		_panel->AddFront(btn);

		btn = std::make_shared<UI::Button>(manager, texman);
		btn->SetText(texman, _lang.editor_save_map.Get());
		btn->Move(200, y);
		btn->eventClick = _commands.exportMap;
		_panel->AddFront(btn);

		btn = std::make_shared<UI::Button>(manager, texman);
		btn->SetText(texman, _lang.editor_map_settings.Get());
		btn->Move(300, y);
		btn->eventClick = std::bind(&MainMenuDlg::OnMapSettings, this);
		_panel->AddFront(btn);
		break;
	default:
		assert(false);
	}
}

void MainMenuDlg::OnTimeStep(UI::LayoutManager &manager, float dt)
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
			_panel->UnlinkAllChildren();
			_panel->Move(0, -_panelFrame->GetHeight());
			_panelTitle.reset();

			if( PT_NONE != _ptype )
			{
				_pstate = PS_APPEARING;
				CreatePanel(manager.GetTextureManager());
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

