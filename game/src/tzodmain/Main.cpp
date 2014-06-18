// Main.cpp

#include <AIManager.h>
#include <constants.h>
#include <globals.h>
#include <gui_desktop.h>
#include <script.h>
#include <Macros.h>
#include <BackgroundIntro.h>
#include <WorldController.h>

#include <config/Config.h>
#include <config/Language.h>

#ifndef NOSOUND
#include <sound/MusicPlayer.h>
#include <sound/sfx.h>
#endif

#include <video/TextureManager.h>
#include <video/RenderOpenGL.h>
//#include <video/RenderDirect3D.h>

//#include <network/Variant.h>
//#include <network/TankClient.h>


#include <gc/Sound.h>
#include <gc/Player.h>
#include <gc/World.h>

#include <fs/FileSystemImpl.h>

#include <core/debug.h>
#include <core/Timer.h>
#include <core/Profiler.h>

#include "res/resource.h"

#include <ConsoleBuffer.h>
#include <GuiManager.h>
#include <GLFW/glfw3.h>

#include <thread>


UI::ConsoleBuffer& GetConsole();

///////////////////////////////////////////////////////////////////////////////

static CounterBase counterDrops("Drops", "Frame drops");
static CounterBase counterTimeBuffer("TimeBuf", "Time buffer");
static CounterBase counterCtrlSent("CtrlSent", "Ctrl packets sent");

///////////////////////////////////////////////////////////////////////////////

struct GlfwInitHelper
{
    GlfwInitHelper()
    {
        if( !glfwInit() )
            throw std::runtime_error("Failed to initialize OpenGL");
    }
    ~GlfwInitHelper()
    {
        glfwTerminate();
    }
};

struct GlfwWindowDeleter
{
    void operator()(GLFWwindow *window)
    {
        glfwDestroyWindow(window);
    }
};


///////////////////////////////////////////////////////////////////////////////

static void OnPrintScreen()
{
//	PLAY(SND_Screenshot, vec2d(0, 0));
    
/*
	// generate a file name

	CreateDirectory(DIR_SCREENSHOTS, NULL);
	SetCurrentDirectory(DIR_SCREENSHOTS);

	int n = g_conf.r_screenshot.GetInt();
	char name[MAX_PATH];
	for(;;)
	{
		sprintf(name, "screenshot%04d.tga", n);

		WIN32_FIND_DATA fd = {0};
		HANDLE h = FindFirstFile(name, &fd);

		if( INVALID_HANDLE_VALUE == h )
			break;

		FindClose(h);
		n++;
	}

	g_conf.r_screenshot.SetInt(n);

	if( !g_render->TakeScreenshot(name) )
	{
		GetConsole().WriteLine(1, "screenshot failed");
//		_MessageArea::Inst()->message("> screen shot error!");
	}
	else
	{
		TRACE("Screenshot '%s'", name);
	}

	SetCurrentDirectory(".."); */
}


namespace
{
	class DesktopFactory : public UI::IWindowFactory
	{
        World &_world;
		WorldController &_worldController;
		AIManager &_aiMgr;
	public:
        DesktopFactory(World &world, WorldController &worldController, AIManager &aiMgr)
            : _world(world)
			, _worldController(worldController)
			, _aiMgr(aiMgr)
        {}
		virtual UI::Window* Create(UI::LayoutManager *manager)
		{
			return new UI::Desktop(manager, _world, _worldController, _aiMgr);
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


static void OnMouseButton(GLFWwindow *window, int button, int action, int mods)
{
    if( auto gui = (UI::LayoutManager *) glfwGetWindowUserPointer(window) )
	{
        UI::Msg msg;
        switch (button)
        {
            case GLFW_MOUSE_BUTTON_LEFT:
                msg = (GLFW_RELEASE == action) ? UI::MSGLBUTTONUP : UI::MSGLBUTTONDOWN;
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                msg = (GLFW_RELEASE == action) ? UI::MSGRBUTTONUP : UI::MSGRBUTTONDOWN;
                break;
            case GLFW_MOUSE_BUTTON_MIDDLE:
                msg = (GLFW_RELEASE == action) ? UI::MSGMBUTTONUP : UI::MSGMBUTTONDOWN;
                break;
            default:
                return;
        }
        double xpos = 0;
        double ypos = 0;
        glfwGetCursorPos(window, &xpos, &ypos);
		gui->ProcessMouse((float) xpos, (float) ypos, 0, msg);
	}
}

static void OnCursorPos(GLFWwindow *window, double xpos, double ypos)
{
    if( auto gui = (UI::LayoutManager *) glfwGetWindowUserPointer(window) )
	{
		gui->ProcessMouse((float) xpos, (float) ypos, 0, UI::MSGMOUSEMOVE);
	}
}

static void OnScroll(GLFWwindow *window, double xoffset, double yoffset)
{
    if( auto gui = (UI::LayoutManager *) glfwGetWindowUserPointer(window) )
    {
        double xpos = 0;
        double ypos = 0;
        glfwGetCursorPos(window, &xpos, &ypos);
        gui->ProcessMouse((float) xpos, (float) ypos, (float) yoffset, UI::MSGMOUSEWHEEL);
    }
}

static void OnKey(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if( auto gui = (UI::LayoutManager *) glfwGetWindowUserPointer(window) )
    {
        gui->ProcessKeys(GLFW_RELEASE == action ? UI::MSGKEYUP : UI::MSGKEYDOWN, key);
        if( GLFW_KEY_PRINT_SCREEN == key && GLFW_PRESS == action )
            OnPrintScreen();
    }
}

static void OnChar(GLFWwindow *window, unsigned int codepoint)
{
    if( auto gui = (UI::LayoutManager *) glfwGetWindowUserPointer(window) )
    {
        if( codepoint < 57344 || codepoint > 63743 ) // ignore Private Use Area characters
        {
            gui->ProcessKeys(UI::MSGCHAR, codepoint);
        }
    }
}

static void OnFramebufferSize(GLFWwindow *window, int width, int height)
{
    auto gui = (UI::LayoutManager *) glfwGetWindowUserPointer(window);
    gui->GetDesktop()->Resize(width, height);
    gui->GetTextureManager().SetCanvasSize(width, height);
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
	srand(time(nullptr));
//	Variant::Init();

#if defined(_DEBUG) && defined(_WIN32) // memory leaks detection
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	GetConsole().SetLog(new ConsoleLog("log.txt"));

	try
	{
        TRACE("%s", TXT_VERSION);

        TRACE("Mount file system");
        g_fs = FS::OSFileSystem::Create("data");
        
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
        
        
        GC_Sound::_countMax = g_conf.s_maxchanels.GetInt();

		{ // TODO: remove explicit appWindow scope
        TRACE("Create GL context");
        GlfwInitHelper __gih;
		const GLFWvidmode *videoMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		std::unique_ptr<GLFWwindow, GlfwWindowDeleter> appWindow;
		appWindow.reset(glfwCreateWindow(g_conf.r_fullscreen.Get() ? videoMode->width : g_conf.r_width.GetInt(),
										 g_conf.r_fullscreen.Get() ? videoMode->height : g_conf.r_height.GetInt(),
										 TXT_VERSION,
										 g_conf.r_fullscreen.Get() ? glfwGetPrimaryMonitor() : nullptr,
										 nullptr));
		if (!appWindow)
			throw std::runtime_error("Failed to create GLFW window");
		
        glfwSetMouseButtonCallback(appWindow.get(), OnMouseButton);
        glfwSetCursorPosCallback(appWindow.get(), OnCursorPos);
        glfwSetScrollCallback(appWindow.get(), OnScroll);
        glfwSetKeyCallback(appWindow.get(), OnKey);
        glfwSetCharCallback(appWindow.get(), OnChar);
        glfwSetFramebufferSizeCallback(appWindow.get(), OnFramebufferSize);
        glfwMakeContextCurrent(appWindow.get());
        
		g_appWindow = appWindow.get();


        { // TODO: remove explicit render scope
        std::unique_ptr<IRender> render = /*g_conf.r_render.GetInt() ? renderCreateDirect3D() :*/ RenderCreateOpenGL();
        g_render = render.get();
        int width;
        int height;
        glfwGetFramebufferSize(appWindow.get(), &width, &height);
        render->OnResizeWnd(width, height);
        
#if !defined NOSOUND
        InitSound(true);
#endif
        
        g_texman = new TextureManager(*render);
        g_texman->SetCanvasSize(width, height);
        if( g_texman->LoadPackage(FILE_TEXTURES, g_fs->Open(FILE_TEXTURES)->QueryMap()) <= 0 )
            TRACE("WARNING: no textures loaded");
        if( g_texman->LoadDirectory(DIR_SKINS, "skin/") <= 0 )
            TRACE("WARNING: no skins found");

        { // FIXME: remove explicit world scope
        World world;
		WorldController worldController(world);
		AIManager aiManager(world);

        TRACE("scripting subsystem initialization");
        g_env.L = script_open(world);
        g_conf->GetRoot()->InitConfigLuaBinding(g_env.L, "conf");
        g_lang->GetRoot()->InitConfigLuaBinding(g_env.L, "lang");
        
        { // FIXME: remove explicit gui scope
        TRACE("GUI subsystem initialization");
        UI::LayoutManager gui(*g_texman, DesktopFactory(world, worldController, aiManager));
        glfwSetWindowUserPointer(appWindow.get(), &gui);
        gui.GetDesktop()->Resize((float) width, (float) height);
        
        TRACE("Running startup script '%s'", FILE_STARTUP);
        if( !script_exec_file(g_env.L, FILE_STARTUP) )
            TRACE("ERROR: in startup script");
        
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
            if (glfwWindowShouldClose(appWindow.get()))
                break;
			
			float dt = timer.GetDt();
			worldController.SendControllerStates(aiManager.ComputeAIState(world, dt));
            gui.TimeStep(dt); // also sends controller state to WorldController
			world.Step(dt);
            
            render->Begin();
            gui.Render();
            render->End();
            
#ifndef NOSOUND
            if( g_music )
            {
                g_music->HandleBufferFilling();
            }
#endif
            
            glfwSwapBuffers(appWindow.get());
        }
        
        
        //
        // Shutdown
        //
        
        TRACE("Shutting down the GUI subsystem");
        } // FIXME: remove explicit gui scope
        
        TRACE("Shutting down the world");
        world.Clear();
        } // FIXME: remove explicit world scope
        
        TRACE("Shutting down the scripting subsystem");
        script_close(g_env.L);
        g_env.L = NULL;
        
        if( g_texman )
        {
            delete g_texman;
            g_texman = NULL;
        }
        
#ifndef NOSOUND
        FreeSound();
#endif
            
        g_render = nullptr;
        TRACE("Shutting down the renderer");
        } // TODO: remove explicit render scope
        
        TRACE("Destroying gl context");
        g_appWindow = nullptr;
		} // TODO: remove explicit appWindow scope
        
        TRACE("Saving config to '" FILE_CONFIG "'");
        if( !g_conf->GetRoot()->Save(FILE_CONFIG) )
        {
            TRACE("Failed to save config file");
        }
        
        TRACE("Unmounting the file system");
        g_fs.reset();

        TRACE("Exit.");
	}
	catch( const std::exception &e )
	{
		GetConsole().Format(SEVERITY_ERROR) << "Error:\n" << e.what();
#ifdef _WIN32
		MessageBoxA(NULL, e.what(), TXT_VERSION, MB_ICONERROR);
#endif
		return 1;
	}
	return 0;
}

// end of file
