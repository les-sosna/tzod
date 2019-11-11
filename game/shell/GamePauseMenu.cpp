#include "GamePauseMenu.h"
#include <cbind/ConfigBinding.h>
#include <loc/Language.h>
#include <ui/Button.h>

static const float c_buttonWidth = 200;
static const float c_buttonHeight = 50;

GamePauseMenu::GamePauseMenu(LangCache &lang, GamePauseMenuCommands commands)
	: _commands(std::move(commands))
{
	SetFlowDirection(UI::FlowDirection::Vertical);
	SetSpacing(20);

	std::shared_ptr<UI::Button> button;

	button = std::make_shared<UI::Button>();
	button->SetFont("font_default");
	button->SetText(ConfBind(lang.game_restart_btn));
	button->Resize(c_buttonWidth, c_buttonHeight);
	button->eventClick = _commands.restartGame;
	AddFront(button);
	SetFocus(button.get());

	button = std::make_shared<UI::Button>();
	button->SetFont("font_default");
	button->SetText(ConfBind(lang.settings_btn));
	button->Resize(c_buttonWidth, c_buttonHeight);
	button->eventClick = _commands.gameSettings;
	AddFront(button);

	button = std::make_shared<UI::Button>();
	button->SetFont("font_default");
	button->SetText(ConfBind(lang.quit_btn));
	button->Resize(c_buttonWidth, c_buttonHeight);
	button->eventClick = _commands.quitGame;
	AddFront(button);
}
