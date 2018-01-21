#pragma once
#ifndef NOSOUND
# include <audio/SoundView.h>
#endif
#include <platetc/UIInputRenderingController.h>
#include <ui/GuiManager.h>
#include <video/TextureManager.h>
#include <memory>

class TzodApp;

class TzodViewImpl
{
public:
	TzodViewImpl(FS::FileSystem &fs, UI::ConsoleBuffer &logger, TzodApp &app, AppWindow &appWindow);

	void Step(float dt);

private:
	TzodApp &_app;
	TextureManager _textureManager;
	UI::TimeStepManager _timeStepManager;
	std::shared_ptr<UI::Window> _desktop;
	UIInputRenderingController _uiInputRenderingController;
#ifndef NOSOUND
	SoundView _soundView;
#endif
};

