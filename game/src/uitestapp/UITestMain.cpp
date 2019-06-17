#include "UITestDesktop.h"
#ifdef _WIN32
#include <fswin/FileSystemWin32.h>
using FileSystem = FS::FileSystemWin32;
#else
#include <fsposix/FileSystemPosix.h>
using FileSystem = FS::FileSystemPosix;
#endif // _WIN32
#include <platetc/UIInputRenderingController.h>
#include <platglfw/GlfwAppWindow.h>
#include <platglfw/Timer.h>
#include <video/TextureManager.h>
#include <ui/GuiManager.h>
#include <exception>
#include <string>
#include <iostream>

static void print_what(const std::exception &e, std::string prefix);

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
	auto fs = std::make_shared<FileSystem>("data");

	GlfwAppWindow appWindow(
		"LibUI Test App",
		false, // fullscreen
		1024, // width
		768 // height
	);

	TextureManager textureManager(appWindow.GetRender());
	textureManager.LoadPackage(appWindow.GetRender(), *fs, ParseDirectory(*fs, "sprites"));

	auto desktop = std::make_shared<UITestDesktop>();

	Timer timer;
	timer.SetMaxDt(0.05f);
	timer.Start();

	UI::TimeStepManager timeStepManager;
	UIInputRenderingController controller(appWindow.GetInput(), textureManager, timeStepManager, desktop);

	while (!appWindow.ShouldClose())
	{
		controller.OnRefresh(appWindow);
		GlfwAppWindow::PollEvents(controller);
		controller.TimeStep(timer.GetDt());
	}

	textureManager.UnloadAllTextures(appWindow.GetRender());

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

