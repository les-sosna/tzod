#include <GlfwPlatform.h>
#include <gui_desktop.h>
#ifndef NOSOUND
#include <audio/SoundView.h>
#endif
//#include <script/script.h>
//#include <script/ScriptHarness.h>
#include <ThemeManager.h>

#include <Config.h>

//#include <network/Variant.h>
//#include <network/TankClient.h>

#include <core/Debug.h>
#include <core/Timer.h>
#include <core/Profiler.h>

#include <../FileSystemImpl.h>
#include <app/AppCfg.h>
#include <app/AppController.h>
#include <app/AppState.h>
#include <app/GameContextBase.h>
#include <loc/Language.h>
#include <ui/GuiManager.h>
#include <ui/UIInput.h>
#include <ui/Clipboard.h>
#include <video/DrawingContext.h>
#include <video/TextureManager.h>
#include <video/RenderOpenGL.h>
//#include <video/RenderDirect3D.h>

#include <GLFW/glfw3.h>
#include <algorithm>
#include <exception>
#include <thread>

///////////////////////////////////////////////////////////////////////////////

static CounterBase counterDrops("Drops", "Frame drops");
static CounterBase counterTimeBuffer("TimeBuf", "Time buffer");
static CounterBase counterCtrlSent("CtrlSent", "Ctrl packets sent");

///////////////////////////////////////////////////////////////////////////////

namespace
{
	class DesktopFactory : public UI::IWindowFactory
	{
		AppState &_appState;
		AppController &_appController;
		FS::FileSystem &_fs;
		std::function<void()> _exitCommand;
	public:
		DesktopFactory(AppState &appState,
			           AppController &appController,
			           FS::FileSystem &fs,
			           std::function<void()> exitCommand)
			: _appState(appState)
			, _appController(appController)
			, _fs(fs)
			, _exitCommand(std::move(exitCommand))
		{}
		UI::Window* Create(UI::LayoutManager *manager) override
		{
			return new Desktop(manager, _appState, _appController, _fs, _exitCommand);
		}
	};

	class ConsoleLog : public UI::IConsoleLog
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
		virtual void WriteLine(int severity, const std::string &str) override final
		{
			if( _file )
			{
				fputs(str.c_str(), _file);
				fputs("\n", _file);
				fflush(_file);
			}
			puts(str.c_str());
		}
		virtual void Release() override final
		{
			delete this;
		}
	};
}

static void LoadConfigNoThrow()
try
{
	// workaround - check if file exists
	if (FILE *f = fopen(FILE_CONFIG, "r"))
	{
		fclose(f);

		TRACE("Loading config '" FILE_CONFIG "'");
		if (!g_conf->Load(FILE_CONFIG))
		{
			GetConsole().Format(1) << "Failed to load config file.";
		}
	}
	else
	{
		TRACE("Config '" FILE_CONFIG "' not found; using defaults");
	}
}
catch (std::exception &e)
{
	TRACE("Could not load config from '" FILE_CONFIG "': %s", e.what());
}

// recursively print exception whats:
static void print_what(const std::exception &e, std::string prefix = std::string())
{
#ifdef _WIN32
	OutputDebugStringA((prefix + e.what() + "\n").c_str());
#endif
	GetConsole().Format(1) << prefix << e.what();
	try {
		std::rethrow_if_nested(e);
	} catch (const std::exception &nested) {
		print_what(nested, prefix + "> ");
	}
}

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
	GetConsole().SetLog(new ConsoleLog("log.txt"));

	TRACE("%s", TXT_VERSION);

	TRACE("Mount file system");
	std::shared_ptr<FS::FileSystem> fs = FS::OSFileSystem::Create("data");

	LoadConfigNoThrow();

	TRACE("Localization init...");
	try
	{
		if( !g_lang->Load(FILE_LANGUAGE) )
		{
			TRACE("couldn't load language file " FILE_CONFIG);
		}
	}
	catch( const std::exception &e )
	{
		TRACE("could not load localization file: %s", e.what());
	}
	setlocale(LC_CTYPE, g_lang.c_locale.Get().c_str());


	TRACE("Create GL context");
	GlfwAppWindow appWindow(TXT_VERSION, g_conf.r_fullscreen.Get(), g_conf.r_width.GetInt(), g_conf.r_height.GetInt());

	glfwMakeContextCurrent(&appWindow.GetGlfwWindow());
	glfwSwapInterval(1);

	std::unique_ptr<IRender> render = /*g_conf.r_render.GetInt() ? renderCreateDirect3D() :*/ RenderCreateOpenGL();
	int width;
	int height;
	glfwGetFramebufferSize(&appWindow.GetGlfwWindow(), &width, &height);
	render->OnResizeWnd(width, height);

	AppState appState;
	TextureManager texman(*render);
	ThemeManager themeManager(appState, *fs, texman);
	if (texman.LoadPackage(FILE_TEXTURES, fs->Open(FILE_TEXTURES)->QueryMap(), *fs) <= 0)
		TRACE("WARNING: no textures loaded");
	if (texman.LoadDirectory(DIR_SKINS, "skin/", *fs) <= 0)
		TRACE("WARNING: no skins found");
	auto exitCommand = std::bind(glfwSetWindowShouldClose, &appWindow.GetGlfwWindow(), 1);
	AppController appController(*fs);
#ifndef NOSOUND
	SoundView soundView(appState, *fs->GetFileSystem(DIR_SOUND));
#endif
	GlfwInput input(appWindow.GetGlfwWindow());
	GlfwClipboard clipboard(appWindow.GetGlfwWindow());
	UI::LayoutManager gui(input,
		                    clipboard,
		                    texman,
		                    DesktopFactory(appState,
		                                    appController,
		                                    *fs,
		                                    exitCommand));
	glfwSetWindowUserPointer(&appWindow.GetGlfwWindow(), &gui);
	gui.GetDesktop()->Resize((float) width, (float) height);

//    g_env.L = gameContext.GetScriptHarness().GetLuaState();
//    g_conf->InitConfigLuaBinding(g_env.L, "conf");
//    g_lang->InitConfigLuaBinding(g_env.L, "lang");
//    TRACE("Running startup script '%s'", FILE_STARTUP);
//    if( !script_exec_file(g_env.L, *fs, FILE_STARTUP) )
//        TRACE("ERROR: in startup script");

	Timer timer;
	timer.SetMaxDt(0.05f);
	timer.Start();
	for(;;)
	{
		if( g_conf.dbg_sleep.GetInt() > 0 && g_conf.dbg_sleep_rand.GetInt() >= 0 )
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(std::min(5000, g_conf.dbg_sleep.GetInt() + rand() % (g_conf.dbg_sleep_rand.GetInt() + 1))));
		}

		//
		// controller pass
		//

		glfwPollEvents();
		if (glfwWindowShouldClose(&appWindow.GetGlfwWindow()))
			break;

		float dt = timer.GetDt();

		gui.TimeStep(dt); // this also sends user controller state to WorldController
		if (GameContextBase *gc = appState.GetGameContext())
			gc->Step(dt * g_conf.sv_speed.GetFloat() / 100);


		//
		// view pass
		//

#ifndef NOSOUND

//        vec2d pos(0, 0);
//        if (!_world.GetList(LIST_cameras).empty())
//        {
//            _world.GetList(LIST_cameras).for_each([&pos](ObjectList::id_type, GC_Object *o)
//            {
//                pos += static_cast<GC_Camera*>(o)->GetCameraPos();
//            });
//        }
//        soundView.SetListenerPos(pos);


		soundView.Step();
#endif

		glfwGetFramebufferSize(&appWindow.GetGlfwWindow(), &width, &height);
		DrawingContext dc(texman, (unsigned int) width, (unsigned int) height);

		render->Begin();
		gui.Render(dc);
		render->End();

		glfwSwapBuffers(&appWindow.GetGlfwWindow());
	}

	TRACE("Saving config to '" FILE_CONFIG "'");
	if( !g_conf->Save(FILE_CONFIG) )
	{
		TRACE("Failed to save config file");
	}

	TRACE("Exit.");
	return 0;
}
catch (const std::exception &e)
{
	print_what(e);
#ifdef _WIN32
	MessageBoxA(nullptr, e.what(), TXT_VERSION, MB_ICONERROR);
#endif
	return 1;
}

// end of file
