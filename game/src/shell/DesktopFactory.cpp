#include "Desktop.h"
#include "inc/shell/DesktopFactory.h"

DesktopFactory::DesktopFactory(AppState &appState,
                               AppController &appController,
                               FS::FileSystem &fs,
                               ConfCache &conf,
                               LangCache &lang,
                               UI::ConsoleBuffer &logger)
	: _appState(appState)
	, _appController(appController)
	, _fs(fs)
	, _conf(conf)
	, _lang(lang)
	, _logger(logger)
{
}

std::shared_ptr<UI::Window> DesktopFactory::Create(UI::LayoutManager &manager, TextureManager &texman)
{
	return std::make_shared<Desktop>(manager, texman, _appState, _appController, _fs, _conf, _lang, _logger);
}
