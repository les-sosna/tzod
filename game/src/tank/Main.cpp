// Main.cpp

#include "stdafx.h"

#include "macros.h"
#include "Level.h"
#include "directx.h"
#include "KeyMapper.h"

#include "config/Config.h"

#include "core/debug.h"
#include "core/Console.h"

#include "video/TextureManager.h"
#include "video/RenderBase.h"

#include "ui/Interface.h"
#include "ui/GuiManager.h"
#include "ui/gui_desktop.h"

#include "gc/Sound.h"

#include "fs/FileSystem.h"

#include "res/resource.h"

///////////////////////////////////////////////////////////////////////////////

static void OnPrintScreen()
{
	PLAY(SND_Screenshot, vec2d(0, 0));

	// определяем № файла

	CreateDirectory(DIR_SCREENSHOTS, NULL);
	SetCurrentDirectory(DIR_SCREENSHOTS);

	int n = g_conf.r_screenshot->GetInt();
	char name[MAX_PATH];
	while (1)
	{
		wsprintf(name, "screenshot%04d.tga", n);

		WIN32_FIND_DATA fd = {0};
		HANDLE h = FindFirstFile(name, &fd);

		if( INVALID_HANDLE_VALUE == h )
			break;

		FindClose(h);
		n++;
	}

	g_conf.r_screenshot->SetInt(n);

	if( !g_render->TakeScreenshot(name) )
	{
		TRACE("ERROR: screen shot failed\n");
//		_MessageArea::Inst()->message("> ошибка!");
	}
	else
	{
		TRACE("Screenshot '%s'\n", name);
	}

	SetCurrentDirectory("..");
}

///////////////////////////////////////////////////////////////////////////////

static void TimeStep(float dt)
{
	if( g_level ) g_level->TimeStep(dt);
	if( g_gui ) g_gui->TimeStep(dt);
}

static void RenderFrame(bool thumbnail)
{
	g_render->Begin();

	if( g_level ) g_level->Render();
	if( g_gui ) g_gui->Render();

	g_render->End(); // display new frame


	// check if print screen key is pressed
	static char _oldRQ = 0;
	if( g_env.envInputs.keys[DIK_SYSRQ] && !_oldRQ )
		OnPrintScreen();
	_oldRQ = g_env.envInputs.keys[DIK_SYSRQ];
}

static void EndFrame()
{
	if( g_level )
	{
		OBJECT_LIST::safe_iterator it = g_level->endframe.safe_begin();
		while( it != g_level->endframe.end() )
		{
			GC_Object* pEF_Obj = *it;
			pEF_Obj->EndFrame();
			++it;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

static UI::Window* CreateDesktopWindow(GuiManager *mgr)
{
	return new UI::Desktop(mgr);
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


	TRACE("Create main app window\n");
	HWND h = CreateWindowEx( 0, TXT_WNDCLASS, TXT_VERSION,
	                       WS_POPUP|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_MAXIMIZEBOX|WS_SYSMENU,
	                       CW_USEDEFAULT, CW_USEDEFAULT, // position
	                       CW_USEDEFAULT, CW_USEDEFAULT, // size
	                       NULL, NULL, hInstance, NULL );

	return h;
}

int APIENTRY WinMain( HINSTANCE hinst,
					  HINSTANCE /*hPrevInstance*/,
					  LPSTR /*lpCmdLine*/,
					  int /*nCmdShow*/ )
{
	g_hInstance = hinst;

	// create the console buffer
	g_console = new ConsoleBuffer(128, 512);

    // print UNIX-style date and time
	time_t ltime;
	char timebuf[26];
    time( &ltime );
    ctime_s(timebuf, 26, &ltime);
	TRACE(" Engine started at %s", timebuf);
	TRACE("--------------------------------------------\n");


	// create main app window
	g_env.hMainWnd = CreateMainWnd(hinst);


	//
	// init file system
	//
	TRACE("Mounting file system\n");
	g_fs = OSFileSystem::Create(".");


	//
	// init config system
	//

	g_config = new ConfVarTable();
	if( !g_config->Load(FILE_CONFIG) )
	{
		TRACE("couldn't load " FILE_CONFIG "\n");

		int result = MessageBox(g_env.hMainWnd, 
			"Syntax error in the config file. Default settings will be used.",
			TXT_VERSION,
			MB_ICONERROR | MB_OKCANCEL);

		if( IDOK != result )
		{
			return 0;
		}
	}
	g_conf.Initialize(g_config);
	if( 0 == g_conf.dm_profiles->GetSize() )
		CreateDefaultProfiles();

	g_env.nNeedCursor  = 0;
	g_env.minimized    = false;
	g_env.camera_x     = 0;
	g_env.camera_y     = 0;

	GC_Sound::_countMax = g_conf.s_maxchanels->GetInt();


	//
	// init common controls
	//

	TRACE("windows common controls initialization\n");
	INITCOMMONCONTROLSEX iccex = { sizeof(INITCOMMONCONTROLSEX), 0 };
	InitCommonControlsEx(&iccex);


	//
	// show graphics mode selection dialog
	//
	if( g_conf.r_askformode->Get() 
		&& IDOK != DialogBox(hinst, (LPCTSTR) IDD_DISPLAY, NULL, (DLGPROC) dlgDisplaySettings) )
	{
		g_fs = NULL; // free the file system
		return 0;
	}


	//
	// init key mapper
	//
	g_keys = new KeyMapper();


	//
	// show main app window
	//
	ShowWindow(g_env.hMainWnd, SW_SHOW);
	UpdateWindow(g_env.hMainWnd);



	//
	// init scripting system
	//

	TRACE("scripting subsystem initialization\n");
	if( NULL == (g_env.hScript = script_open()) )
	{
		TRACE("FAILED\n");
		return -1;
	}
	InitConfigLuaBinding(LS(g_env.hScript), g_config, "conf");


	// create global timer
	Timer *timer = new Timer();
	timer->SetMaxDt(MAX_DT);


#ifndef _DEBUG
	BOOL bGeneralFault = FALSE;
#endif


	if( SUCCEEDED(InitAll(g_env.hMainWnd)) )
	{
		// init texture manager
		g_texman = new TextureManager;
		LoadSurfaces();

		timeBeginPeriod(1);

#ifndef _DEBUG
		try
		{
		;
#endif
		// init GUI
		TRACE("GUI subsystem initialization\n");
		g_gui = new GuiManager(CreateDesktopWindow);
		g_render->OnResizeWnd();
		g_gui->Resize((float) g_render->GetWidth(), (float) g_render->GetHeight());


		TRACE("Execing startup script '%s'\n", FILE_STARTUP);
		if( !script_exec_file(g_env.hScript, FILE_STARTUP) )
		{
			TRACE("ERROR: in startup script\n");
			MessageBoxT(g_env.hMainWnd, "startup script error", MB_ICONERROR);
		}

		timer->Start();

		MSG msg;
		while(true) // цикл обработки сообщений
		{
			if( PeekMessage(&msg, NULL, 0, 0, TRUE) )
			{
				if( WM_QUIT == msg.message ) break;

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				ReadImmediateData();  // чтение состояния устройств ввода
				//------------------------------
				if( g_level && g_env.envInputs.keys[DIK_F8] && g_level->_modeEditor )
				{
					DialogBox(g_hInstance, (LPCTSTR)IDD_MAP_SETTINGS, g_env.hMainWnd, (DLGPROC) dlgMapSettings);
					continue;
				}
				else if( g_env.envInputs.keys[DIK_LALT] && g_env.envInputs.keys[DIK_TAB] )
				{
					if( g_level ) g_level->Pause(true);
					ShowWindow(g_env.hMainWnd, SW_MINIMIZE);
					if( g_level ) g_level->Pause(false);
					continue;
				}
				else if( g_env.envInputs.keys[DIK_F12] )
				{
					if(g_level) g_level->Pause(true);
					DialogBox(g_hInstance, (LPCTSTR)IDD_OPTIONS, g_env.hMainWnd, (DLGPROC) dlgOptions);
					if(g_level) g_level->Pause(false);
					continue;
				}
				else if( g_env.envInputs.keys[DIK_LALT] && g_env.envInputs.keys[DIK_F4] )
				{
					TRACE("Alt + F4 has been pressed. Destroying the main app window\n");
					DestroyWindow(g_env.hMainWnd);
					continue;
				}
				//--------------------------------
				TimeStep(timer->GetDt());
				RenderFrame(false);
				EndFrame();
			}
		}

#ifndef _DEBUG
		} // end of try block
		catch(...) // general error handling
		{
			TRACE("GENERAL FAULT\n");
			bGeneralFault = TRUE;
		}
#endif

		TRACE("Shutting down GUI subsystem\n");
		SAFE_DELETE(g_gui);
		timeEndPeriod(1);
	} // end if( SUCCEEDED(InitAll(hWnd)) )
	else
	{
		MessageBoxT(NULL, "Ошибка инициализации", MB_ICONERROR);
	}


	SAFE_DELETE(timer);


	FreeDirectInput();

	g_texman->UnloadAllTextures();
	SAFE_DELETE(g_texman);

	TRACE("Shutting down the renderer\n");
	SAFE_RELEASE(g_render);


#if !defined NOSOUND
	FreeDirectSound();
#endif

#ifndef _DEBUG
	if( bGeneralFault )
	{
		MessageBoxT(NULL, "Критическая ошибка", MB_ICONERROR);
	}
#endif

	// script engine cleanup
	TRACE("Shutting down the scripting subsystem\n");
	script_close(g_env.hScript);
	g_env.hScript = NULL;

	// key mapper
	SAFE_DELETE(g_keys);

	// config
	TRACE("Saving config to '" FILE_CONFIG "'\n");
	g_config->Save(FILE_CONFIG);
	SAFE_DELETE(g_config);

	// clean up the file system
	TRACE("Unmounting the file system\n");
	g_fs = NULL;

	TRACE("Exit.\n");

	// free the console buffer
	SAFE_DELETE(g_console);

	Sleep(500);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// end of file
