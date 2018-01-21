#pragma once
#include <memory>

class TzodApp;
class TzodViewImpl;
struct AppWindow;

namespace FS
{
	class FileSystem;
}

namespace UI
{
	class ConsoleBuffer;
}

class TzodView
{
public:
	TzodView(FS::FileSystem &fs, UI::ConsoleBuffer &logger, TzodApp &app, AppWindow &appWindow);
	~TzodView();

	void Step(float dt);

private:
	AppWindow &_appWindow;
	std::unique_ptr<TzodViewImpl> _impl;
};
