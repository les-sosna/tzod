#include "UITestDesktop.h"

#include <fs/FileSystem.h>
#include <platglfw/GlfwAppWindow.h>
#include <platglfw/Timer.h>
#include <video/DrawingContext.h>
#include <video/TextureManager.h>
#include <ui/InputContext.h>
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
	GlfwAppWindow appWindow(
		"LibUI Test App",
		false, // fullscreen
		1024, // width
		768 // height
	);

	std::shared_ptr<FS::FileSystem> fs = FS::CreateOSFileSystem("data");

	TextureManager textureManager(appWindow.GetRender());
	if (textureManager.LoadPackage(ParsePackage(FILE_TEXTURES, fs->Open(FILE_TEXTURES)->QueryMap(), *fs)) <= 0)
		std::cerr << "WARNING: no textures loaded" << std::endl;

	UI::InputContext inputContext(appWindow.GetInput(), appWindow.GetClipboard());
	UI::LayoutManager gui(textureManager, inputContext);

	auto desktop = std::make_shared<UITestDesktop>(gui, textureManager);
	gui.SetDesktop(desktop);

	Timer timer;
	timer.SetMaxDt(0.05f);
	timer.Start();
	for (GlfwAppWindow::PollEvents(); !appWindow.ShouldClose(); GlfwAppWindow::PollEvents())
	{
		float dt = timer.GetDt();

		gui.TimeStep(dt);

		unsigned int width = appWindow.GetPixelWidth();
		unsigned int height = appWindow.GetPixelHeight();
		float layoutScale = appWindow.GetLayoutScale();

		DrawingContext dc(textureManager, width, height);
		appWindow.GetRender().Begin();
		gui.Render(*desktop, vec2d{ static_cast<float>(width), static_cast<float>(height) }, layoutScale, dc);
		appWindow.GetRender().End();

		appWindow.Present();
	}

	return 0;
}
catch (const std::exception &e)
{
	print_what(/*s_logger,*/ e, "");
	return 1;
}

// recursively print exception whats:
static void print_what(/*UI::ConsoleBuffer &logger, */const std::exception &e, std::string prefix)
{
#ifdef _WIN32
	OutputDebugStringA((prefix + e.what() + "\n").c_str());
#endif
	//	logger.Format(1) << prefix << e.what();
	try {
		std::rethrow_if_nested(e);
	}
	catch (const std::exception &nested) {
		print_what(/*logger, */nested, prefix + "> ");
	}
}

