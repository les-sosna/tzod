#pragma once
#include <platetc/UIInputRenderingController.h>
#include <ui/GuiManager.h>
#include <video/TextureManager.h>
#include <deque>
#include <memory>

class TzodApp;
class SoundView;
namespace Plat
{
	class ConsoleBuffer;
}

class TzodViewImpl
{
public:
	TzodViewImpl(FS::FileSystem &fs, Plat::ConsoleBuffer &logger, TzodApp &app, Plat::AppWindow &appWindow);
	~TzodViewImpl();

	void Step(float dt);

private:
	TzodApp &_app;
	TextureManager _textureManager;
	UI::TimeStepManager _timeStepManager;
	std::shared_ptr<UI::Window> _desktop;
	UIInputRenderingController _uiInputRenderingController;
	std::deque<float> _movingAverageWindow;
	std::deque<float> _movingMedianWindow;
#ifndef NOSOUND
	std::unique_ptr<SoundView> _soundView;
#endif
};

