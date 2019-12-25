#pragma once
#include <ui/StackLayout.h>
#include <functional>

class LangCache;

struct GamePauseMenuCommands
{
	std::function<void()> restartGame;
	std::function<void()> gameSettings;
	std::function<void()> quitGame;
};

class GamePauseMenu final
	: public UI::StackLayout
{
public:
	GamePauseMenu(LangCache &lang, GamePauseMenuCommands commands);

private:
	GamePauseMenuCommands _commands;
};
