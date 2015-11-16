#include <../FileSystemImpl.h> // wtf dot dot
#include <app/tzod.h>
#include <app/View.h>
#include <as/AppCfg.h>
#include <plat/GlfwAppWindow.h>
#include <plat/Timer.h>
#include <ui/ConsoleBuffer.h>

#include <exception>

namespace
{
	class ConsoleLog final
		: public UI::IConsoleLog
	{
		FILE *_file;
		ConsoleLog(const ConsoleLog&) = delete;
		ConsoleLog& operator= (const ConsoleLog&) = delete;
	public:
		explicit ConsoleLog(const char *filename)
			: _file(fopen(filename, "w"))
		{
		}
		virtual ~ConsoleLog()
		{
			if( _file )
				fclose(_file);
		}

		// IConsoleLog
		void WriteLine(int severity, const std::string &str) override
		{
			if( _file )
			{
				fputs(str.c_str(), _file);
				fputs("\n", _file);
				fflush(_file);
			}
			puts(str.c_str());
		}
		void Release() override
		{
			delete this;
		}
	};
}


// recursively print exception whats:
static void print_what(UI::ConsoleBuffer &logger, const std::exception &e, std::string prefix = std::string())
{
#ifdef _WIN32
	OutputDebugStringA((prefix + e.what() + "\n").c_str());
#endif
	logger.Format(1) << prefix << e.what();
	try {
		std::rethrow_if_nested(e);
	} catch (const std::exception &nested) {
		print_what(logger, nested, prefix + "> ");
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
	std::shared_ptr<FS::FileSystem> fs = FS::OSFileSystem::Create("data");

	TzodApp app(*fs, logger);

	logger.Printf(0, "Create GL context");
	GlfwAppWindow appWindow(
		TXT_VERSION,
		false, // conf.r_fullscreen.Get(),
		1024, // conf.r_width.GetInt(),
		768 // conf.r_height.GetInt()
	);

	TzodView view(*fs, logger, app, appWindow);

	Timer timer;
	timer.SetMaxDt(0.05f);
	timer.Start();
	for (GlfwAppWindow::PollEvents(); !appWindow.ShouldClose(); GlfwAppWindow::PollEvents())
	{
		float dt = timer.GetDt();

		// controller pass
		view.Step(dt); // this also sends user controller state to WorldController
		app.Step(dt);

		// view pass
		view.Render(appWindow);
		appWindow.Present();
	}

	app.Exit();

	logger.Printf(0, "Normal exit.");

	return 0;
}
catch (const std::exception &e)
{
	print_what(s_logger, e);
#ifdef _WIN32
	MessageBoxA(nullptr, e.what(), TXT_VERSION, MB_ICONERROR);
#endif
	return 1;
}

