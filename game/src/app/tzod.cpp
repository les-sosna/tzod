#include "inc/app/tzod.h"
#include <as/AppController.h>
#include <as/AppState.h>
#include <ctx/GameContextBase.h>
#include <loc/Language.h>
#include <shell/Config.h>
#include <ui/ConsoleBuffer.h>

#define FILE_CONFIG      "config.cfg"
#define FILE_LANGUAGE    "data/lang.cfg"

struct TzodAppImpl
{
	explicit TzodAppImpl(FS::FileSystem &fs)
		: appController(fs)
	{}

	ConfCache conf;
	LangCache lang;
	AppState appState;
	AppController appController;
};

template <class T>
static void LoadConfigNoThrow(T &confRoot, UI::ConsoleBuffer &logger, const char *filename)
try
{
	logger.Printf(0, "Loading config '%s'", filename);
	if (!confRoot->Load(filename))
	{
		logger.Format(1) << "Failed to load a config file; using defaults";
	}
}
catch (const std::exception &e)
{
	logger.Printf(1, "Could not load config '%s': %s", filename, e.what());
}

TzodApp::TzodApp(FS::FileSystem &fs, UI::ConsoleBuffer &logger)
	: _fs(fs)
	, _logger(logger)
	, _impl(new TzodAppImpl(fs))
{
	LoadConfigNoThrow(_impl->conf, logger, FILE_CONFIG),
	LoadConfigNoThrow(_impl->lang, logger, FILE_LANGUAGE),
	setlocale(LC_CTYPE, _impl->lang.c_locale.Get().c_str());
}

TzodApp::~TzodApp()
{
}

AppState& TzodApp::GetAppState()
{
	return _impl->appState;
}

AppController& TzodApp::GetAppController()
{
	return _impl->appController;
}

ConfCache& TzodApp::GetConf()
{
	return _impl->conf;
}

LangCache& TzodApp::GetLang()
{
	return _impl->lang;
}

void TzodApp::Step(float dt)
{
	if (GameContextBase *gc = _impl->appState.GetGameContext())
	{
		gc->Step(dt);
	}
}

void TzodApp::Exit()
{
	_logger.Printf(0, "Saving config to '" FILE_CONFIG "'");
	if (!_impl->conf->Save(FILE_CONFIG))
	{
		_logger.Printf(1, "Failed to save config file");
	}
}
