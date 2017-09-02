#include "inc/app/tzod.h"
#include "CombinedConfig.h"
#include <as/AppController.h>
#include <as/AppState.h>
#include <ctx/GameContextBase.h>
#include <fs/FileSystem.h>
#include <fs/StreamWrapper.h>
#include <loc/Language.h>
#include <ui/ConsoleBuffer.h>

#define FILE_CONFIG      "user/config.cfg"
#define FILE_DMCAMPAIGN  "dmcampaign.cfg"

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
static void LoadConfigNoThrow(FS::FileSystem &fs, T &confRoot, UI::ConsoleBuffer &logger, const std::string &filename)
try
{
	logger.Printf(0, "Loading config '%s'", filename.c_str());
	if (!confRoot->Load(*fs.Open(filename)->QueryMap(), filename.c_str()))
	{
		logger.Format(1) << "Failed to load a config file. Using defaults.";
	}
}
catch (const std::exception &e)
{
	logger.Printf(1, "Could not load config '%s': %s", filename.c_str(), e.what());
}

TzodApp::TzodApp(FS::FileSystem &fs, UI::ConsoleBuffer &logger, const char *language)
	: _fs(fs)
	, _logger(logger)
	, _impl(new TzodAppImpl(fs))
{
	LoadConfigNoThrow(fs, _impl->combinedConfig, logger, FILE_CONFIG);
	LoadConfigNoThrow(fs, _impl->dmCampaign, logger, FILE_DMCAMPAIGN);
	if (language)
	{
		auto it = s_localizations.find(language);
		if (s_localizations.end() != it)
		{
			LoadConfigNoThrow(fs, _impl->lang, logger, it->second.c_str());
		}
	}
	setlocale(LC_CTYPE, std::string(_impl->lang.c_locale.Get()).c_str());
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
	auto s = _fs.Open(FILE_CONFIG, FS::ModeWrite)->QueryStream();
	_impl->combinedConfig->Save(FS::OutStreamWrapper(*s));
}
