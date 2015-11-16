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

class AppState;
class AppController;
class ConfCache;
class LangCache;

class TzodApp
{
public:
	TzodApp(FS::FileSystem &fs, UI::ConsoleBuffer &logger);
	~TzodApp();

	AppState& GetAppState();
	AppController& GetAppController();
	ConfCache& GetConf();
	LangCache& GetLang();

	void Step(float dt);
	void Exit();

private:
	FS::FileSystem &_fs;
	UI::ConsoleBuffer &_logger;
	std::unique_ptr<struct TzodAppImpl> _impl;
};
