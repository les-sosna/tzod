#include "inc/app/tzod.h"
#include "CombinedConfig.h"
#include <as/AppConfig.h>
#include <as/AppController.h>
#include <as/AppState.h>
#include <ctx/GameContextBase.h>
#include <loc/Language.h>
#include <shell/Config.h>
#include <ui/ConsoleBuffer.h>

#define FILE_CONFIG      "config.cfg"
#define FILE_DMCAMPAIGN  "data/dmcampaign.cfg"

static std::map<std::string, std::string> s_localizations = {
    {"ru", "data/lang.cfg"}
};

struct TzodAppImpl
{
	explicit TzodAppImpl(FS::FileSystem &fs)
		: appController(fs)
	{}

	CombinedConfig combinedConfig;
	DMCampaign dmCampaign;
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

TzodApp::TzodApp(FS::FileSystem &fs, UI::ConsoleBuffer &logger, const char *language)
	: _logger(logger)
	, _impl(new TzodAppImpl(fs))
{
    LoadConfigNoThrow(_impl->combinedConfig, logger, FILE_CONFIG);
    LoadConfigNoThrow(_impl->dmCampaign, logger, FILE_DMCAMPAIGN);
    if (language)
    {
        auto it = s_localizations.find(language);
        if (s_localizations.end() != it)
        {
            LoadConfigNoThrow(_impl->lang, logger, it->second.c_str());
        }
    }
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

AppConfig& TzodApp::GetAppConfig()
{
	return _impl->combinedConfig.game;
}

ShellConfig& TzodApp::GetShellConfig()
{
	return _impl->combinedConfig.shell;
}

LangCache& TzodApp::GetLang()
{
	return _impl->lang;
}

DMCampaign& TzodApp::GetDMCampaign()
{
	return _impl->dmCampaign;
}

void TzodApp::Step(float dt)
{
	_impl->appController.Step(_impl->appState, _impl->combinedConfig.game, dt);
}

void TzodApp::Exit()
{
	_logger.Printf(0, "Saving config to '" FILE_CONFIG "'");
	if (!_impl->combinedConfig->Save(FILE_CONFIG))
	{
		_logger.Printf(1, "Failed to save config file");
	}
}
