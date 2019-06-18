#pragma once
#include <platetc/UIInputRenderingController.h>
#include <ui/GuiManager.h>
#include <video/TextureManager.h>
#include <memory>

class TzodApp;
class SoundView;
namespace Plat
{
	struct AppWindowCommandClose;
	class ConsoleBuffer;
}

struct TzodViewImpl
{
	TzodViewImpl(FS::FileSystem &fs, Plat::AppWindowCommandClose *cmdClose, Plat::ConsoleBuffer &logger, Plat::Input& input, IRender& render, TzodApp& app);
	~TzodViewImpl();

	TextureManager textureManager;
	UI::TimeStepManager timeStepManager;
	std::shared_ptr<UI::Window> desktop;
	UIInputRenderingController uiInputRenderingController;
#ifndef NOSOUND
	std::unique_ptr<SoundView> soundView;
#endif
};

