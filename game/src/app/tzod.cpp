#include "inc/app/tzod.h"
#include "CombinedConfig.h"
#include <as/AppController.h>
#include <as/AppState.h>
#include <ctx/GameContextBase.h>
#include <fs/FileSystem.h>
#include <fs/StreamWrapper.h>
#include <loc/Language.h>
#include <plat/ConsoleBuffer.h>

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
static void LoadConfigNoThrow(FS::FileSystem &fs, T &confRoot, Plat::ConsoleBuffer &logger, const std::string &filename)
try
{
	if (confRoot->Load(*fs.Open(filename)->QueryMap(), filename.c_str()))
	{
		logger.Printf(0, "Config loaded: %s", filename.c_str());
	}
	else
	{
		logger.Format(1) << "Failed to load a config file. Using defaults.";
	}
}
catch (const std::exception &e)
{
	logger.Printf(1, "Could not load config: %s", e.what());
}

TzodApp::TzodApp(FS::FileSystem &fs, Plat::ConsoleBuffer &logger, const char *language)
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
	bool configChanged = false;
	_impl->appController.Step(_impl->appState, _impl->combinedConfig.game, dt, &configChanged);

	if (configChanged)
	{
		SaveConfig();
	}
}

void TzodApp::SaveConfig()
{
	try
	{
		auto s = _fs.Open(FILE_CONFIG, FS::ModeWrite)->QueryStream();
		FS::OutStreamWrapper wrapper(*s);
		_impl->combinedConfig->Save(wrapper);
		_logger.Printf(0, "Config saved: " FILE_CONFIG);
	}
	catch(const std::exception &e)
	{
		_logger.Printf(0, "Failed to save config: %s", e.what());
	}
}
