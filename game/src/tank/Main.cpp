// Main.cpp

#include "stdafx.h"

#include "script.h"
#include "macros.h"
#include "Level.h"
#include "directx.h"
#include "InputManager.h"

#include "config/Config.h"
#include "config/Language.h"

#include "sound/MusicPlayer.h"

#include "core/debug.h"
#include "core/Application.h"
#include "core/Timer.h"

#include "video/TextureManager.h"
#include "video/RenderOpenGL.h"
#include "video/RenderDirect3D.h"

#include "network/TankClient.h"
#include "network/TankServer.h"
#include "network/Variant.h"

#include "ui/Interface.h"
#include "ui/GuiManager.h"
#include "ui/gui_desktop.h"

#include "gc/Sound.h"

#include "fs/FileSystem.h"

#include "res/resource.h"


///////////////////////////////////////////////////////////////////////////////

class ZodApp : public AppBase
{
public:
	ZodApp();
	virtual ~ZodApp();

	virtual bool Pre();
	virtual void Idle();
	virtual void Post();

private:
	Timer timer;
	SafePtr<InputManager> _inputMgr;

//	SafePtr<FileSystem>   _fs;
};

///////////////////////////////////////////////////////////////////////////////

static void OnPrintScreen()
{
	PLAY(SND_Screenshot, vec2d(0, 0));

	// определяем № файла

	CreateDirectory(DIR_SCREENSHOTS, NULL);
	SetCurrentDirectory(DIR_SCREENSHOTS);

	int n = g_conf->r_screenshot->GetInt();
	char name[MAX_PATH];
	for(;;)
	{
		wsprintf(name, "screenshot%04d.tga", n);

		WIN32_FIND_DATA fd = {0};
		HANDLE h = FindFirstFile(name, &fd);

		if( INVALID_HANDLE_VALUE == h )
			break;

		FindClose(h);
		n++;
	}

	g_conf->r_screenshot->SetInt(n);

	if( !g_render->TakeScreenshot(name) )
	{
		GetConsole().WriteLine(1, "screenshot failed");
//		_MessageArea::Inst()->message("> screen shot error!");
	}
	else
	{
		TRACE("Screenshot '%s'", name);
	}

	SetCurrentDirectory("..");
}

///////////////////////////////////////////////////////////////////////////////

static void TimeStep(float dt)
{
	assert(g_level);
	g_level->TimeStep(dt);
	if( g_gui ) g_gui->TimeStep(dt);
}

static void RenderFrame(bool thumbnail)
{
	assert(g_level);
	assert(g_render);

	g_render->Begin();

	if( !g_level->IsEmpty() ) g_level->Render();
	if( g_gui ) g_gui->Render();

	g_render->End(); // display new frame


	// check if print screen key is pressed
	static char _oldRQ = 0;
	if( g_env.envInputs.keys[DIK_SYSRQ] && !_oldRQ )
		OnPrintScreen();
	_oldRQ = g_env.envInputs.keys[DIK_SYSRQ];
}

///////////////////////////////////////////////////////////////////////////////

namespace
{
	class DesktopFactory : public UI::IWindowFactory
	{
	public:
		virtual UI::Window* Create(UI::LayoutManager *manager)
		{
			return new UI::Desktop(manager);
		}
	};
}

static HWND CreateMainWnd(HINSTANCE hInstance)
{
	WNDCLASS wc = {0};

	//
	// register class
	//
	wc.lpszClassName = TXT_WNDCLASS;
	wc.lpfnWndProc   = (WNDPROC) WndProc;
	wc.style         = CS_VREDRAW | CS_HREDRAW;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon(hInstance, (LPCTSTR) IDI_BIG);
	wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = NULL;
	RegisterClass(&wc);


	HWND h = CreateWindowEx( 0, TXT_WNDCLASS, TXT_VERSION,
	                       WS_POPUP|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_MAXIMIZEBOX|WS_SYSMENU,
	                       CW_USEDEFAULT, CW_USEDEFAULT, // position
	                       CW_USEDEFAULT, CW_USEDEFAULT, // size
	                       NULL, NULL, hInstance, NULL );
	TRACE("Created main app window");

	return h;
}

int APIENTRY WinMain( HINSTANCE, // hInstance
                      HINSTANCE, // hPrevInstance
                      LPSTR, // lpCmdLine
                      int // nCmdShow
){
	TCHAR buf[MAX_PATH];
	GetModuleFileName(NULL, buf, MAX_PATH);

	HANDLE hFile = CreateFile(buf, FILE_READ_DATA, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
	HANDLE hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	void *data = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
	DWORD size = GetFileSize(hFile, NULL);

	MD5_CTX md5;
	MD5Init(&md5);
	MD5Update(&md5, (const char *) data, size);
	MD5Final(&md5);

	UnmapViewOfFile(data);
	CloseHandle(hMap);
	CloseHandle(hFile);

	memcpy(&g_md5, md5.digest, 16);


	ZodApp app;
	return app.Run();
}

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
ZodApp::ZodApp()
{
	assert(!g_app);
	g_app = this;
}

ZodApp::~ZodApp()
{
	assert(this == g_app);
	g_app = NULL;
}

bool ZodApp::Pre()
{
	srand( GetTickCount() );
	Variant::Init();

	// print UNIX-style date and time
	time_t ltime;
	char timebuf[26];
	time(&ltime);
	ctime_s(timebuf, 26, &ltime);
	TRACE("ZOD Engine started at %s", timebuf);
	TRACE("----------------------------------------------");
	TRACE("%s", TXT_VERSION);


	g_env.pause = 0;


	// create main app window
	g_env.hMainWnd = CreateMainWnd(GetModuleHandle(NULL));


	//
	// init file system
	//
	TRACE("Mounting file system...");
	g_fs = FS::OSFileSystem::Create("data");


	//
	// init config system
	//

	try
	{
		if( !g_conf.GetRoot()->Load(FILE_CONFIG) )
		{
			int result = MessageBox(g_env.hMainWnd,
				"Syntax error in the config file (see log). Default settings will be used.",
				TXT_VERSION,
				MB_ICONERROR | MB_OKCANCEL);

			if( IDOK != result )
			{
				return false;
			}
		}
	}
	catch( std::exception &e )
	{
		TRACE("Could not load config file: %s", e.what());
	}
	g_conf.GetAccessor(); // force accessor creation


	//
	// init localization
	//
	TRACE("Localization init...");
	try
	{
		if( !g_lang.GetRoot()->Load(FILE_LANGUAGE) )
		{
			TRACE("couldn't load language file " FILE_CONFIG);

			int result = MessageBox(g_env.hMainWnd,
				"Syntax error in the language file (see log). Continue with default (English) language?",
				TXT_VERSION,
				MB_ICONERROR | MB_OKCANCEL);

			if( IDOK != result )
			{
				return false;
			}
		}
	}
	catch( const std::exception &e )
	{
		TRACE("could not load localization file: %s", e.what());
	}
	g_lang.GetAccessor(); // force accessor creation
	setlocale(LC_CTYPE, g_lang->c_locale->Get().c_str());


	// set up the environment
	g_env.nNeedCursor  = 0;
	g_env.minimized    = false;
	g_env.camera_x     = 0;
	g_env.camera_y     = 0;

	GC_Sound::_countMax = g_conf->s_maxchanels->GetInt();


	//
	// init common controls
	//

	TRACE("windows common controls initialization");
	INITCOMMONCONTROLSEX iccex = { sizeof(INITCOMMONCONTROLSEX), 0 };
	InitCommonControlsEx(&iccex);


	//
	// show graphics mode selection dialog
	//
	if( g_conf->r_askformode->Get()
		&& IDOK != DialogBox(GetModuleHandle(NULL), (LPCTSTR) IDD_DISPLAY, NULL, (DLGPROC) dlgDisplaySettings) )
	{
		g_fs = NULL; // free the file system
		return false;
	}
	else
	{
		g_render = g_conf->r_render->GetInt() ? renderCreateDirect3D() : renderCreateOpenGL();
	}


	//
	// show main app window
	//
	ShowWindow(g_env.hMainWnd, SW_SHOW);
	UpdateWindow(g_env.hMainWnd);



	//
	// init scripting system
	//

	TRACE("scripting subsystem initialization");
	if( NULL == (g_env.L = script_open()) )
	{
		TRACE(" ->FAILED");
		return false;
	}
	InitConfigLuaBinding(g_env.L, g_conf.GetRoot(), "conf");
	InitConfigLuaBinding(g_env.L, g_lang.GetRoot(), "lang");


	timer.SetMaxDt(MAX_DT);

	// init render
	assert(g_render);
	DisplayMode dm;
	dm.Width         = g_conf->r_width->GetInt();
	dm.Height        = g_conf->r_height->GetInt();
	dm.RefreshRate   = g_conf->r_freq->GetInt();
	dm.BitsPerPixel  = g_conf->r_bpp->GetInt();
	if( !g_render->Init(g_env.hMainWnd, &dm, g_conf->r_fullscreen->Get()) )
	{
		return false;
	}


#if !defined NOSOUND
	// init sound
	try
	{
		if( FAILED(InitDirectSound(g_env.hMainWnd, true)) )
		{
			MessageBox(g_env.hMainWnd, "Direct Sound init error", TXT_VERSION, MB_ICONERROR|MB_OK);
		}
	}
	catch( const std::exception &e )
	{
		std::ostringstream ss;
		ss << "DirectSound init: " << e.what();
		MessageBox(g_env.hMainWnd, ss.str().c_str(), TXT_VERSION, MB_ICONERROR|MB_OK);
		return false;
	}
#endif


	_inputMgr = WrapRawPtr(new InputManager(g_env.hMainWnd));

	// init texture manager
	g_texman = new TextureManager;
	g_texman->SetCanvasSize(g_render->GetWidth(), g_render->GetHeight());
	try
	{
		if( g_texman->LoadPackage(FILE_TEXTURES, g_fs->Open(FILE_TEXTURES)->QueryMap()) <= 0 )
		{
			TRACE("WARNING: no textures loaded");
			MessageBox(g_env.hMainWnd, "There are no textures loaded", TXT_VERSION, MB_ICONERROR);
		}
		if( g_texman->LoadDirectory(DIR_SKINS, "skin/") <= 0 )
		{
			TRACE("WARNING: no skins found");
			MessageBox(g_env.hMainWnd, "There are no skins found", TXT_VERSION, MB_ICONERROR);
		}
	}
	catch( const std::exception &e )
	{
		GetConsole().WriteLine(1, e.what());
		delete g_texman;
		g_texman = NULL;
		return false;
	}

	// init world
	g_level = WrapRawPtr(new Level());

	// init GUI
	TRACE("GUI subsystem initialization");
	g_gui = new UI::LayoutManager(&DesktopFactory());
	g_render->OnResizeWnd();
	g_gui->GetDesktop()->Resize((float) g_render->GetWidth(), (float) g_render->GetHeight());


	TRACE("Running startup script '%s'", FILE_STARTUP);
	if( !script_exec_file(g_env.L, FILE_STARTUP) )
	{
		TRACE("ERROR: in startup script");
		MessageBox(g_env.hMainWnd, "startup script error", TXT_VERSION, MB_ICONERROR);
	}

	g_level->_gameType = GT_INTRO;

	timer.Start();

	return true;
}

void ZodApp::Idle()
{
	_inputMgr->InquireInputDevices();

	//if( g_env.envInputs.keys[DIK_LALT] && g_env.envInputs.keys[DIK_TAB] )
	//{
	////	PauseGame(true);
	//	ShowWindow(g_env.hMainWnd, SW_MINIMIZE);
	//	return;
	//}
	//else
	//if( g_env.envInputs.keys[DIK_LALT] && g_env.envInputs.keys[DIK_F4] )
	//{
	//	TRACE("Alt + F4 has been pressed. Destroying the main app window");
	//	DestroyWindow(g_env.hMainWnd);
	//	return;
	//}

	TimeStep(timer.GetDt());
	RenderFrame(false);

	if( g_music )
	{
		g_music->HandleBufferFilling();
	}

	if( g_conf->dbg_sleep->GetInt() > 0 && g_conf->dbg_sleep_rand->GetInt() >= 0 )
	{
		Sleep(std::min(5000, g_conf->dbg_sleep->GetInt() + rand() % (g_conf->dbg_sleep_rand->GetInt() + 1)));
	}
}

void ZodApp::Post()
{
	TRACE("Shutting down GUI subsystem");
	SAFE_DELETE(g_gui);

	SAFE_DELETE(g_client);
	SAFE_DELETE(g_server);

	// destroy level
	g_level = NULL;

	// release input devices
	_inputMgr = NULL;

	if( g_texman ) g_texman->UnloadAllTextures();
	SAFE_DELETE(g_texman);

	TRACE("Shutting down the renderer");
	if( g_render )
	{
		g_render->Release();
		g_render = NULL;
	}


#ifndef NOSOUND
	FreeDirectSound();
#endif

	// script engine cleanup
	if( g_env.L )
	{
		TRACE("Shutting down the scripting subsystem");
		script_close(g_env.L);
		g_env.L = NULL;
	}

	// config
	TRACE("Saving config to '" FILE_CONFIG "'");
	if( !g_conf.GetRoot()->Save(FILE_CONFIG) )
	{
		MessageBox(NULL, "Failed to save config file", TXT_VERSION, MB_ICONERROR);
	}

	// clean up the file system
	TRACE("Unmounting the file system");
	g_fs = NULL;

	TRACE("Exit.");
}



///////////////////////////////////////////////////////////////////////////////
// end of file
