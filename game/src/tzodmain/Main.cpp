#include <app/tzod.h>
#include <app/View.h>
#include <as/AppConstants.h>
#include <fs/FileSystem.h>
#include <platglfw/GlfwAppWindow.h>
#include <platglfw/Timer.h>
#include <ui/ConsoleBuffer.h>

#include <exception>
#include <fstream>
#include <iostream>

namespace
{
	class ConsoleLog final
		: public UI::IConsoleLog
	{
		std::ofstream _file;
	public:
		ConsoleLog(const ConsoleLog&) = delete;
		ConsoleLog& operator= (const ConsoleLog&) = delete;

		explicit ConsoleLog(const char *filename)
			: _file(filename, std::ios::out | std::ios::trunc)
		{
		}

		// IConsoleLog
		void WriteLine(int severity, std::string_view str) override
		{
			_file << str << std::endl;
			std::cout << str << std::endl;
		}
		void Release() override
		{
			delete this;
		}
	};
}

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


static UI::ConsoleBuffer s_logger(100, 500);

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
	UI::ConsoleBuffer &logger = s_logger;

	logger.SetLog(new ConsoleLog("log.txt"));
	logger.Printf(0, "%s", TXT_VERSION);
	logger.Printf(0, "Mount file system");
	std::shared_ptr<FS::FileSystem> fs = FS::CreateOSFileSystem("data");

	TzodApp app(*fs, logger);

	logger.Printf(0, "Create GL context");
	GlfwAppWindow appWindow(
		TXT_VERSION,
		false, // conf.r_fullscreen.Get(),
		1024, //app.GetShellConfig().r_width.GetInt(),
		768 //app.GetShellConfig().r_height.GetInt()
	);

	TzodView view(*fs, logger, app, appWindow);

	Timer timer;
	timer.SetMaxDt(0.05f);
	timer.Start();
	while (!appWindow.ShouldClose())
	{
		GlfwAppWindow::PollEvents();
		view.Step(timer.GetDt());
	}

	app.Exit();

	logger.Printf(0, "Normal exit.");

	return 0;
}
catch (const std::exception &e)
{
	std::ostringstream os;
	print_what(os, e);
	s_logger.Format(1) << os.str();
#ifdef _WIN32
	OutputDebugStringA(os.str().c_str());
	MessageBoxA(nullptr, os.str().c_str(), TXT_VERSION, MB_ICONERROR);
#endif
	return 1;
}
