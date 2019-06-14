#pragma once
#include <deque>
#include <memory>

struct IRender;
class TzodApp;

namespace FS
{
	class FileSystem;
}

namespace Plat
{
	struct AppWindowCommandClose;
	struct AppWindowInputSink;
	class ConsoleBuffer;
}

class TzodView final
{
public:
	TzodView(FS::FileSystem &fs, Plat::ConsoleBuffer &logger, TzodApp &app, Plat::AppWindowCommandClose* cmdClose);
	~TzodView();

	void Step(TzodApp& app, float dt);
	Plat::AppWindowInputSink& GetAppWindowInputSink();

private:
	std::unique_ptr<struct TzodViewImpl> _impl;
	std::deque<float> _movingAverageWindow;
	std::deque<float> _movingMedianWindow;
};
