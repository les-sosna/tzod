// Main.cpp

#include <AppState.h>
#include <constants.h>
#include <GameContext.h>
#include <GlfwPlatform.h>
#include <gui_desktop.h>
#ifndef NOSOUND
#include <SoundView.h>
#endif
#include <script/script.h>
#include <script/ScriptHarness.h>
#include <ThemeManager.h>

#include <config/Config.h>
#include <config/Language.h>

//#include <network/Variant.h>
//#include <network/TankClient.h>

#include <gclua/lgcmod.h>

#include <core/Debug.h>
#include <core/Timer.h>
#include <core/Profiler.h>

#include <../FileSystemImpl.h>
#include <GLFW/glfw3.h>
#include <ui/GuiManager.h>
#include <ui/UIInput.h>
#include <ui/Clipboard.h>
#include <video/DrawingContext.h>
#include <video/TextureManager.h>
#include <video/RenderOpenGL.h>
//#include <video/RenderDirect3D.h>

#include <exception>
#include <thread>
#include <numeric>

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
		FS::FileSystem &_fs;
		std::function<void()> _exitCommand;
	public:
        DesktopFactory(AppState &appState,
					   FS::FileSystem &fs,
					   std::function<void()> exitCommand)
            : _appState(appState)
			, _fs(fs)
			, _exitCommand(std::move(exitCommand))
        {}
		virtual UI::Window* Create(UI::LayoutManager *manager)
		{
			return new UI::Desktop(manager, _appState, _fs, _exitCommand);
		}
	};

	class ConsoleLog : public UI::IConsoleLog
	{
		FILE *_file;
		ConsoleLog(const ConsoleLog&); // no copy
		ConsoleLog& operator= (const ConsoleLog&);
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

// recursively print exception whats:
static void print_what(const std::exception& e, std::string prefix = std::string())
{
	GetConsole().Format(SEVERITY_ERROR) << prefix << e.what();
#ifndef _MSC_VER
	try {
		std::rethrow_if_nested(e);
	} catch (const std::exception& nested) {
		print_what(nested, prefix + "> ");
	}
#endif
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
{
	srand((unsigned int) time(nullptr));
//	Variant::Init();

#if defined(_DEBUG) && defined(_WIN32) // memory leaks detection
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	GetConsole().SetLog(new ConsoleLog("log.txt"));

	try
	{
        TRACE("%s", TXT_VERSION);

        TRACE("Mount file system");
		std::shared_ptr<FS::FileSystem> fs = FS::OSFileSystem::Create("data");
        
        // load config
        try
        {
            // workaround - check if file exists
            if( FILE *f = fopen(FILE_CONFIG, "r") )
            {
                fclose(f);
                
                if( !g_conf->GetRoot()->Load(FILE_CONFIG) )
                {
                    GetConsole().Format(SEVERITY_ERROR) << "Failed to load config file.";
                }
            }
            
        }
        catch( std::exception &e )
        {
            TRACE("Could not load config file: %s", e.what());
        }
        
        TRACE("Localization init...");
        try
        {
            if( !g_lang->GetRoot()->Load(FILE_LANGUAGE) )
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
		
		ThemeManager themeManager(*fs);
		TextureManager texman(*render);
		if (texman.LoadPackage(FILE_TEXTURES, fs->Open(FILE_TEXTURES)->QueryMap(), *fs) <= 0)
			TRACE("WARNING: no textures loaded");
		if (texman.LoadDirectory(DIR_SKINS, "skin/", *fs) <= 0)
			TRACE("WARNING: no skins found");
		auto exitCommand = std::bind(glfwSetWindowShouldClose, &appWindow.GetGlfwWindow(), 1);
		AppState appState;
#ifndef NOSOUND
		SoundView soundView(appState, *fs->GetFileSystem(DIR_SOUND));
#endif
		GlfwInput input(appWindow.GetGlfwWindow());
		GlfwClipboard clipboard(appWindow.GetGlfwWindow());
        UI::LayoutManager gui(input,
							  clipboard,
							  texman,
							  DesktopFactory(appState,
											 *fs,
											 exitCommand));
        glfwSetWindowUserPointer(&appWindow.GetGlfwWindow(), &gui);
        gui.GetDesktop()->Resize((float) width, (float) height);
        
//        g_env.L = gameContext.GetScriptHarness().GetLuaState();
//        g_conf->GetRoot()->InitConfigLuaBinding(g_env.L, "conf");
//        g_lang->GetRoot()->InitConfigLuaBinding(g_env.L, "lang");
//        TRACE("Running startup script '%s'", FILE_STARTUP);
//        if( !script_exec_file(g_env.L, *fs, FILE_STARTUP) )
//            TRACE("ERROR: in startup script");

		std::deque<float> movingAverageWindow;
		std::deque<float> movingMedianWindow;
        
        Timer timer;
        timer.SetMaxDt(MAX_DT);
        timer.Start();
        for(;;)
        {
            if( g_conf.dbg_sleep.GetInt() > 0 && g_conf.dbg_sleep_rand.GetInt() >= 0 )
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(std::min(5000, g_conf.dbg_sleep.GetInt() + rand() % (g_conf.dbg_sleep_rand.GetInt() + 1))));
            }
            
            glfwPollEvents();
            if (glfwWindowShouldClose(&appWindow.GetGlfwWindow()))
                break;
			
			float dt = timer.GetDt();
			
			// moving average
			movingAverageWindow.push_back(dt);
			if (movingAverageWindow.size() > 8)
				movingAverageWindow.pop_front();
			float mean = std::accumulate(movingAverageWindow.begin(), movingAverageWindow.end(), 0.0f) / (float)movingAverageWindow.size();
			// moving median of moving average
			movingMedianWindow.push_back(mean);
			if (movingMedianWindow.size() > 100)
				movingMedianWindow.pop_front();
			float buf[100];
			std::copy(movingMedianWindow.begin(), movingMedianWindow.end(), buf);
			std::nth_element(buf, buf + movingMedianWindow.size() / 2, buf + movingMedianWindow.size());
			float median = buf[movingMedianWindow.size() / 2];
			
			gui.TimeStep(dt); // this also sends user controller state to WorldController
			if (GameContextBase *gc = appState.GetGameContext())
				gc->Step(median * g_conf.sv_speed.GetFloat() / 100);
#ifndef NOSOUND
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
        if( !g_conf->GetRoot()->Save(FILE_CONFIG) )
        {
            TRACE("Failed to save config file");
        }
        
        TRACE("Exit.");
	}
	catch( const std::exception &e )
	{
		print_what(e);
#ifdef _WIN32
		MessageBoxA(NULL, e.what(), TXT_VERSION, MB_ICONERROR);
#endif
		return 1;
	}
	return 0;
}

// end of file
