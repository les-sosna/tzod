#include "UITestDesktop.h"

#include <fs/FileSystem.h>
#include <platetc/UIInputRenderingController.h>
#include <platglfw/GlfwAppWindow.h>
#include <platglfw/Timer.h>
#include <video/TextureManager.h>
#include <ui/GuiManager.h>
#include <exception>
#include <string>
#include <iostream>

#define FILE_TEXTURES "scripts/textures.lua"

static void print_what(/*UI::ConsoleBuffer &logger, */const std::exception &e, std::string prefix);

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <windef.h>
int APIENTRY WinMain( HINSTANCE, // hInstance
                      HINSTANCE, // hPrevInstance
                      LPSTR, // lpCmdLine
                      int) // nCmdShow
#else
int main(int, const char**)
#endif
try
{
	std::shared_ptr<FS::FileSystem> fs = FS::CreateOSFileSystem("data");

	GlfwAppWindow appWindow(
		"LibUI Test App",
		false, // fullscreen
		1024, // width
		768 // height
	);

	TextureManager textureManager(appWindow.GetRender());
	if (textureManager.LoadPackage(ParsePackage(FILE_TEXTURES, fs->Open(FILE_TEXTURES)->QueryMap(), *fs)) <= 0)
		std::cerr << "WARNING: no textures loaded" << std::endl;

	UI::TimeStepManager timeStepManager;
	auto desktop = std::make_shared<UITestDesktop>();

	Timer timer;
	timer.SetMaxDt(0.05f);
	timer.Start();

	UIInputRenderingController controller(appWindow, textureManager, timeStepManager, desktop);

	while (!appWindow.ShouldClose())
	{
		GlfwAppWindow::PollEvents();

		controller.TimeStep(timer.GetDt());
		controller.OnRefresh();
	}

	return 0;
}
catch (const std::exception &e)
{
	print_what(/*s_logger,*/ e, "");
	return 1;
}

static void print_what(const std::exception &e, std::string prefix)
{
#ifdef _WIN32
	OutputDebugStringA((prefix + e.what() + "\n").c_str());
#endif
	try {
		std::rethrow_if_nested(e);
	}
	catch (const std::exception &nested) {
		print_what(nested, prefix + "> ");
	}
}

