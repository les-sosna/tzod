#pragma once
#include <memory>

namespace FS
{
	class FileSystem;
}

namespace Plat
{
	class ConsoleBuffer;
}

struct EventPump;
struct AppWindow;

class AppConfig;
class AppState;
class AppController;
class ShellConfig;
class LangCache;
class DMCampaign;

class TzodApp final
{
public:
	TzodApp(FS::FileSystem &fs, Plat::ConsoleBuffer &logger, const char *language = nullptr);
	~TzodApp();

	AppState& GetAppState();
	AppController& GetAppController();
	AppConfig& GetAppConfig();
	ShellConfig& GetShellConfig();
	LangCache& GetLang();
	DMCampaign &GetDMCampaign();

	void Step(float dt);
	void Exit();

private:
	FS::FileSystem &_fs;
	Plat::ConsoleBuffer &_logger;
	std::unique_ptr<struct TzodAppImpl> _impl;
};
