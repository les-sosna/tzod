// Main.cpp

#include <globals.h>
#include <gui_desktop.h>
#include <script.h>
#include <Macros.h>
#include <World.h>
#include <BackgroundIntro.h>

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


#include <gc/Camera.h>
#include <gc/Sound.h>
#include <gc/Player.h>

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


static void Idle(World &world, float dt);


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
	public:
        DesktopFactory(World &world)
            : _world(world)
        {}
		virtual UI::Window* Create(UI::LayoutManager *manager)
		{
			return new UI::Desktop(manager, _world);
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
    if( g_gui )
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
		g_gui->ProcessMouse((float) xpos, (float) ypos, 0, msg);
	}
}

static void OnCursorPos(GLFWwindow *window, double xpos, double ypos)
{
    if( g_gui )
	{
		g_gui->ProcessMouse((float) xpos, (float) ypos, 0, UI::MSGMOUSEMOVE);
	}
}

static void OnScroll(GLFWwindow *window, double xoffset, double yoffset)
{
    double xpos = 0;
    double ypos = 0;
    glfwGetCursorPos(window, &xpos, &ypos);
    g_gui->ProcessMouse((float) xpos, (float) ypos, (float) yoffset, UI::MSGMOUSEWHEEL);
}

static void OnKey(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    g_gui->ProcessKeys(GLFW_RELEASE == action ? UI::MSGKEYUP : UI::MSGKEYDOWN, key);
    if( GLFW_KEY_PRINT_SCREEN == key && GLFW_PRESS == action )
		OnPrintScreen();

}

static void OnChar(GLFWwindow *window, unsigned int codepoint)
{
    if( codepoint < 57344 || codepoint > 63743 ) // ignore Private Use Area characters
	{
        g_gui->ProcessKeys(UI::MSGCHAR, codepoint);
    }
}

static void OnFramebufferSize(GLFWwindow *window, int width, int height)
{
    g_render->OnResizeWnd(Point{width, height});
    g_texman->SetCanvasSize(width, width);
    g_gui->GetDesktop()->Resize(width, height);
    if (g_level)
        GC_Camera::UpdateLayout(*g_level);
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

        TRACE("Create GL context");
        GlfwInitHelper __gih;
        g_appWindow = glfwCreateWindow(g_conf.r_width.GetInt(),
                                       g_conf.r_height.GetInt(),
                                       TXT_VERSION,
                                       /*g_conf.r_fullscreen.Get() ? glfwGetPrimaryMonitor() :*/ nullptr,
                                       nullptr);
        glfwSetMouseButtonCallback(g_appWindow, OnMouseButton);
        glfwSetCursorPosCallback(g_appWindow, OnCursorPos);
        glfwSetScrollCallback(g_appWindow, OnScroll);
        glfwSetKeyCallback(g_appWindow, OnKey);
        glfwSetCharCallback(g_appWindow, OnChar);
        glfwSetFramebufferSizeCallback(g_appWindow, OnFramebufferSize);
        glfwMakeContextCurrent(g_appWindow);


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
        
        // init render
        g_render = /*g_conf.r_render.GetInt() ? renderCreateDirect3D() :*/ RenderCreateOpenGL();
        int width;
        int height;
        glfwGetFramebufferSize(g_appWindow, &width, &height);
        g_render->OnResizeWnd(Point{width, height});
        
#if !defined NOSOUND
        InitSound(true);
#endif
        
        g_texman = new TextureManager;
        g_texman->SetCanvasSize(g_render->GetWidth(), g_render->GetHeight());
        if( g_texman->LoadPackage(FILE_TEXTURES, g_fs->Open(FILE_TEXTURES)->QueryMap()) <= 0 )
            TRACE("WARNING: no textures loaded");
        if( g_texman->LoadDirectory(DIR_SKINS, "skin/") <= 0 )
            TRACE("WARNING: no skins found");

        { // FIXME: remove explicit world scope
        World world;
        g_level = &world;

        TRACE("scripting subsystem initialization");
        g_env.L = script_open(world);
        g_conf->GetRoot()->InitConfigLuaBinding(g_env.L, "conf");
        g_lang->GetRoot()->InitConfigLuaBinding(g_env.L, "lang");
        
        TRACE("GUI subsystem initialization");
        g_gui = new UI::LayoutManager(DesktopFactory(world));
        g_gui->GetDesktop()->Resize((float) g_render->GetWidth(), (float) g_render->GetHeight());
        
        TRACE("Running startup script '%s'", FILE_STARTUP);
        if( !script_exec_file(g_env.L, FILE_STARTUP) )
            TRACE("ERROR: in startup script");
        
        Timer timer;
        timer.SetMaxDt(MAX_DT);
        timer.Start();
        for(;;)
        {
            glfwPollEvents();
            if (glfwWindowShouldClose(g_appWindow))
                break;
            Idle(world, timer.GetDt());
            glfwSwapBuffers(g_appWindow);
        }
        
        
        //
        // Shutdown
        //
        
        TRACE("Shutting down the GUI subsystem");
        SAFE_DELETE(g_gui);
        
        TRACE("Shutting down the world");
        world.Clear();
        g_level = nullptr;
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
        if( g_render )
        {
            TRACE("Shutting down the renderer");
            g_render.reset();
        }
        
        TRACE("Saving config to '" FILE_CONFIG "'");
        if( !g_conf->GetRoot()->Save(FILE_CONFIG) )
        {
            TRACE("Failed to save config file");
        }
        
        TRACE("Unmounting the file system");
        g_fs = NULL;

        TRACE("Destroying gl context");
        glfwDestroyWindow(g_appWindow);
        g_appWindow = nullptr;
        
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

///////////////////////////////////////////////////////////////////////////////


void Idle(World &world, float dt)
{
	world._defaultCamera.HandleMovement(world._sx, world._sy, (float) g_render->GetWidth(), (float) g_render->GetHeight());

    g_gui->TimeStep(dt);
    
	assert(g_render);
	g_render->Begin();
    g_gui->Render();
	g_render->End();
    

#ifndef NOSOUND
	if( g_music )
	{
		g_music->HandleBufferFilling();
	}
#endif

	if( g_conf.dbg_sleep.GetInt() > 0 && g_conf.dbg_sleep_rand.GetInt() >= 0 )
	{
        std::this_thread::sleep_for(std::chrono::milliseconds(
            std::min(5000, g_conf.dbg_sleep.GetInt() + rand() % (g_conf.dbg_sleep_rand.GetInt() + 1))));
	}
}


///////////////////////////////////////////////////////////////////////////////
// end of file
