#include "Desktop.h"
#include "inc/shell/DesktopFactory.h"

DesktopFactory::DesktopFactory(AppState &appState,
                               AppController &appController,
                               FS::FileSystem &fs,
                               ConfCache &conf,
                               LangCache &lang,
                               UI::ConsoleBuffer &logger,
                               std::function<void()> exitCommand)
	: _appState(appState)
	, _appController(appController)
	, _fs(fs)
	, _conf(conf)
	, _lang(lang)
	, _logger(logger)
	, _exitCommand(std::move(exitCommand))
{
}

UI::Window* DesktopFactory::Create(UI::LayoutManager *manager)
{
	return new Desktop(manager, _appState, _appController, _fs, _conf, _lang, _logger, _exitCommand);
}
