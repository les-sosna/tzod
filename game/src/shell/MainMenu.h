#pragma once
#include <ui/StackLayout.h>
#include <functional>

namespace FS
{
	class FileSystem;
}

namespace Plat
{
	class ConsoleBuffer;
}

class LangCache;

struct MainMenuCommands
{
	std::function<void()> singlePlayer;
	std::function<void()> splitScreen;
	std::function<void()> openMap;
	std::function<void()> exportMap;
	std::function<void()> gameSettings;
	std::function<void()> close;
	std::function<void()> quitGame;
};

class MainMenuDlg final
	: public UI::StackLayout
{
public:
	MainMenuDlg(LangCache &lang, MainMenuCommands commands);

private:
	MainMenuCommands _commands;
};
