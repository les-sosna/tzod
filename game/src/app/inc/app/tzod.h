#pragma once
#include <memory>

namespace FS
{
	class FileSystem;
}

namespace UI
{
	class ConsoleBuffer;
}

struct EventPump;
struct AppWindow;

class AppConfig;
class AppState;
class AppController;
class ConfCache;
class LangCache;
class DMCampaign;

class TzodApp
{
public:
	TzodApp(FS::FileSystem &fs, UI::ConsoleBuffer &logger, const char *language = nullptr);
	~TzodApp();

	AppState& GetAppState();
	AppController& GetAppController();
	AppConfig& GetAppConfig();
	ConfCache& GetConf();
	LangCache& GetLang();
	DMCampaign &GetDMCampaign();

	void Step(float dt);
	void Exit();

private:
	UI::ConsoleBuffer &_logger;
	std::unique_ptr<struct TzodAppImpl> _impl;
};
