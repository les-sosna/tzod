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

	button = UI::Button::Create(this, texman, _lang.single_player_btn.Get(), 0, y);
	button->SetIcon(texman, "ui/play");
	button->Resize(buttonWidth, buttonHeight);
	button->eventClick = _commands.newDM;

	button = UI::Button::Create(this, texman, _lang.editor_btn.Get(), (GetWidth() - buttonWidth) / 2, y);
	button->SetIcon(texman, "ui/editor");
	button->Resize(buttonWidth, buttonHeight);
	button->eventClick = std::bind(&MainMenuDlg::OnEditor, this);

	button = UI::Button::Create(this, texman, _lang.settings_btn.Get(), GetWidth() - buttonWidth, y);
	button->SetIcon(texman, "ui/settings");
	button->Resize(buttonWidth, buttonHeight);
	button->eventClick = _commands.gameSettings;

	_panelFrame = std::make_shared<Window>(manager);
	_panelFrame->SetDrawBackground(false);
	_panelFrame->SetDrawBorder(false);
	_panelFrame->SetClipChildren(true);
	_panelFrame->Move(0, GetHeight() + 40);
	_panelFrame->Resize(GetWidth(), 64);
	AddFront(_panelFrame);

	_panel = std::make_shared<Window>(manager);
	_panel->SetDrawBackground(false);
	_panel->SetDrawBorder(false);
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
	case UI::Key::F12:
		_commands.gameSettings();
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

void MainMenuDlg::CreatePanel(TextureManager &texman)
{
	_panelTitle = UI::Text::Create(_panel.get(), texman, 0, 0, "", alignTextLT);
	_panelTitle->SetFont(texman, "font_default");

	float y = _panelTitle->GetHeight() + _panelTitle->GetY() + 10;
	std::shared_ptr<UI::Button> btn;

	switch( _ptype )
	{
	case PT_EDITOR:
		_panelTitle->SetText(texman, _lang.editor_title.Get());
		UI::Button::Create(_panel.get(), texman, _lang.editor_new_map.Get(), 0, y)->eventClick = _commands.newMap;
		UI::Button::Create(_panel.get(), texman, _lang.editor_load_map.Get(), 100, y)->eventClick = _commands.openMap;
		UI::Button::Create(_panel.get(), texman, _lang.editor_save_map.Get(), 200, y)->eventClick = _commands.exportMap;
		btn = UI::Button::Create(_panel.get(), texman, _lang.editor_map_settings.Get(), 300, y);
		btn->eventClick = std::bind(&MainMenuDlg::OnMapSettings, this);
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

