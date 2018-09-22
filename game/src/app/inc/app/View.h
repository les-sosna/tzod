#pragma once
#include <memory>

class TzodApp;
class TzodViewImpl;

namespace FS
{
	class FileSystem;
}

namespace Plat
{
	struct AppWindow;
}

namespace UI
{
	class ConsoleBuffer;
}

class TzodView
{
public:
	TzodView(FS::FileSystem &fs, UI::ConsoleBuffer &logger, TzodApp &app, Plat::AppWindow &appWindow);
	~TzodView();

	void Step(float dt);

private:
	Plat::AppWindow &_appWindow;
	std::unique_ptr<TzodViewImpl> _impl;
};
