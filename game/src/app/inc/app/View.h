#pragma once
#include <memory>

class TzodApp;
struct AppWindow;
struct IRender;

namespace FS
{
	class FileSystem;
}

namespace UI
{
	class LayoutManager;
	class ConsoleBuffer;
}

class TzodView
{
public:
	TzodView(FS::FileSystem &fs,
		UI::ConsoleBuffer &logger,
		TzodApp &app,
		AppWindow &appWindow);
	~TzodView();

	void Step(float dt);
	void Render(AppWindow &appWindow);

	UI::LayoutManager& GetGui();

private:
	AppWindow &_appWindow;
	std::unique_ptr<IRender> _render;
	std::unique_ptr<struct TzodViewImpl> _impl;
};
