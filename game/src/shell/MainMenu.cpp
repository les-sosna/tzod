#include "MainMenu.h"
#include "ConfigBinding.h"
#include <loc/Language.h>
#include <ui/Button.h>

static const float c_buttonWidth = 200;
static const float c_buttonHeight = 128;


MainMenuDlg::MainMenuDlg(UI::LayoutManager &manager,
                         TextureManager &texman,
                         LangCache &lang,
                         MainMenuCommands commands)
  : StackLayout(manager)
  , _lang(lang)
  , _commands(std::move(commands))
{
	SetFlowDirection(UI::FlowDirection::Horizontal);
	SetSpacing(20);

	Resize(640, c_buttonHeight);

	std::shared_ptr<UI::Button> button;

	button = std::make_shared<UI::Button>(manager, texman);
	button->SetText(ConfBind(_lang.single_player_btn));
	button->SetIcon(manager, texman, "ui/play");
	button->Resize(c_buttonWidth, c_buttonHeight);
	button->eventClick = _commands.newDM;
	AddFront(button);

	button = std::make_shared<UI::Button>(manager, texman);
	button->SetText(ConfBind(_lang.editor_btn));
	button->SetIcon(manager, texman, "ui/editor");
	button->Resize(c_buttonWidth, c_buttonHeight);
	button->eventClick = _commands.openMap;
	AddFront(button);

	button = std::make_shared<UI::Button>(manager, texman);
	button->SetText(ConfBind(_lang.settings_btn));
	button->SetIcon(manager, texman, "ui/settings");
	button->Resize(c_buttonWidth, c_buttonHeight);
	button->eventClick = _commands.gameSettings;
	AddFront(button);
}

