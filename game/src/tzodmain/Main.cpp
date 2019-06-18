#include "ConsoleLog.h"
#include <app/tzod.h>
#include <app/Version.h>
#include <app/View.h>
#ifdef _WIN32
#include <fswin/FileSystemWin32.h>
using FileSystem = FS::FileSystemWin32;
#else
#include <fsposix/FileSystemPosix.h>
using FileSystem = FS::FileSystemPosix;
#endif // _WIN32
#include <plat/Folders.h>
#include <platglfw/GlfwAppWindow.h>
#include <platglfw/Timer.h>
#include <exception>

static void print_what(std::ostream &os, const std::exception &e, std::string prefix = std::string())
{
	os << prefix << e.what() << std::endl;
	try {
		std::rethrow_if_nested(e);
	}
	catch (const std::exception &nested) {
		print_what(os, nested, prefix + "> ");
	}
}

static Plat::ConsoleBuffer s_logger(100, 500);

//static long xxx = _CrtSetBreakAlloc(12649);

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
	srand((unsigned int) time(nullptr));
//	Variant::Init();

#if defined(_DEBUG) && defined(_WIN32) // memory leaks detection
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	s_logger.SetLog(new ConsoleLog("log.txt"));
	s_logger.Printf(0, "%s", TZOD_VERSION);

	auto fs = std::make_shared<FileSystem>(Plat::GetBundleResourcesFolder())->GetFileSystem("data");
	auto user = std::make_shared<FileSystem>(Plat::GetAppDataFolder())->GetFileSystem("Tank Zone of Death", true);
	fs->Mount("user", user);

	TzodApp app(*fs, s_logger);

	s_logger.Printf(0, "Creating GL context");
	GlfwAppWindow appWindow(
		TZOD_VERSION,
		false, // conf.r_fullscreen.Get(),
		1024, //app.GetShellConfig().r_width.GetInt(),
		768 //app.GetShellConfig().r_height.GetInt()
	);

	TzodView view(*fs, s_logger, app, appWindow);
	Timer timer;
	timer.SetMaxDt(0.05f);
	timer.Start();
	do {
		view.GetAppWindowInputSink().OnRefresh(appWindow);
		GlfwAppWindow::PollEvents(view.GetAppWindowInputSink());
		view.Step(app, timer.GetDt());
	} while (!appWindow.ShouldClose());

	app.SaveConfig();

	s_logger.Printf(0, "Normal exit.");

	return 0;
}
catch (const std::exception &e)
{
	std::ostringstream os;
	print_what(os, e);
	s_logger.Format(1) << os.str();
#ifdef _WIN32
	OutputDebugStringA(os.str().c_str());
	MessageBoxA(nullptr, os.str().c_str(), TZOD_VERSION, MB_ICONERROR);
#endif
	return 1;
}
