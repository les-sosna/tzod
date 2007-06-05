// Main.cpp

#include "stdafx.h"

#include "macros.h"
#include "Level.h"
#include "options.h"
#include "directx.h"

#include "config/Config.h"

#include "core/debug.h"
#include "core/Console.h"

#include "video/TextureManager.h"
#include "video/RenderBase.h"

#include "ui/Interface.h"
#include "ui/GuiManager.h"
#include "ui/gui_desktop.h"

#include "gc/Camera.h"
#include "gc/Light.h"
#include "gc/Sound.h"
#include "gc/RigidBody.h"
#include "gc/GameClasses.h"


#include "fs/FileSystem.h"

#include "res/resource.h"

/////////////////////////////////////////////////////////////

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
		char zero[4] = {0};
		if( n < 1000 ) zero[0] = '0';
		if( n <  100 ) zero[1] = '0';
		if( n <   10 ) zero[2] = '0';

		wsprintf(name, "screenshot%s%d.tga", zero, n);

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
		TRACE("ERROR: take screenshot failed\n");
//		_MessageArea::Inst()->message("> ошибка!");
	}
	else
	{
		TRACE("Screenshot '%s'\n", name);
	}

	SetCurrentDirectory("..");
}

/////////////////////////////////////////////////////////////

static void TimeStep(float dt)
{
	if( g_level ) g_level->TimeStep(dt);
	if( g_gui ) g_gui->TimeStep(dt);
}

static void RenderFrame(bool thumbnail)
{
	g_render->Begin();

	if( g_level )
	{
		//
		// определение активных камер и выбор конфигурации дисплея
		//

		GC_Camera *pMaxShake = NULL;

		if( g_render->getXsize() >= (int)g_level->_sx &&
			g_render->getYsize() >= (int)g_level->_sy )
		{
			float max_shake = 0;
			FOREACH( cameras, GC_Camera, pCamera )
			{
				if( !pCamera->IsActive() ) continue;
				if( pCamera->GetShake() >= max_shake )
				{
					pMaxShake = pCamera;
					max_shake = pCamera->GetShake();
				}
			}
		}

		int count = 0;
		FOREACH( cameras, GC_Camera, pCamera )
		{
			if( pMaxShake ) pCamera = pMaxShake;
			if( !pCamera->IsActive() ) continue;


			//
			// рендеринг освещения
			//

			g_render->setMode(RM_LIGHT);
			pCamera->Select();
			if( g_conf.sv_nightmode->Get() )
			{
				float xmin = (float) __max(0, g_env.camera_x );
				float ymin = (float) __max(0, g_env.camera_y );
				float xmax = __min(g_level->_sx, (float) g_env.camera_x +
					(float) g_render->getViewportXsize() / pCamera->_zoom );
				float ymax = __min(g_level->_sy, (float) g_env.camera_y +
					(float) g_render->getViewportYsize() / pCamera->_zoom );

				FOREACH( lights, GC_Light, pLight )
				{
					_ASSERT(!pLight->IsKilled());
					if( pLight->_pos.x + pLight->GetRenderRadius() > xmin &&
						pLight->_pos.x - pLight->GetRenderRadius() < xmax &&
						pLight->_pos.y + pLight->GetRenderRadius() > ymin &&
						pLight->_pos.y - pLight->GetRenderRadius() < ymax )
					{
						pLight->Shine();
					}
				}
			}


			//
			// paint all objects
			//

			g_render->setMode(RM_WORLD);

			// paint background texture
			_Background::Inst()->Draw();

			for( int z = 0; z < Z_COUNT; ++z )
			{
				// loop over gridsets
				for( int lev = 0; lev < 4; ++lev )
				{
					static const int dx[] = {0, LOCATION_SIZE/2, 0, LOCATION_SIZE/2};
					static const int dy[] = {0, 0, LOCATION_SIZE/2, LOCATION_SIZE/2};

					int xmin = __max(0, (g_env.camera_x - dx[lev]) / LOCATION_SIZE);
					int ymin = __max(0, (g_env.camera_y - dy[lev]) / LOCATION_SIZE);
					int xmax = __min(g_level->_locations_x - 1,
						((g_env.camera_x + int((float) g_render->getViewportXsize() /
						pCamera->_zoom)) - dx[lev]) / LOCATION_SIZE);
					int ymax = __min(g_level->_locations_y - 1,
						((g_env.camera_y + int((float) g_render->getViewportYsize() /
						pCamera->_zoom)) - dy[lev]) / LOCATION_SIZE);

					for( int x = xmin; x <= xmax; ++x )
					for( int y = ymin; y <= ymax; ++y )
					{
						FOREACH( z_grids[z](lev).element(x,y), GC_2dSprite, object )
						{
							_ASSERT(!object->IsKilled());
							object->Draw();
							_ASSERT(!object->IsKilled());
						}
					}
				} // loop over gridsets


				// loop over globals
				FOREACH( z_globals[z], GC_2dSprite, object )
				{
					_ASSERT(!object->IsKilled());
					object->Draw();
					_ASSERT(!object->IsKilled());
				}
			}

			if( pMaxShake ) break;
		}	// cameras

/*
		//
		// paint Z_SCREEN layer
		//

		if( !thumbnail )
		{
			g_render->setMode(RM_INTERFACE);

			FOREACH( z_globals[Z_COUNT-1], GC_2dSprite, object )
			{
				_ASSERT(!object->IsKilled());
				object->Draw();
				_ASSERT(!object->IsKilled());
			}
		}
*/
	}

	if( g_gui )
	{
		g_render->setMode(RM_INTERFACE);
		g_gui->Draw();
	}


	// display new frame
	g_render->End();


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
	TRACE("Create main app window\n");
    HWND hWnd = CreateWindowEx( 0, TXT_WNDCLASS, TXT_VERSION,
                           WS_POPUP|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_MAXIMIZEBOX|WS_SYSMENU,
						   CW_USEDEFAULT, CW_USEDEFAULT, // position
  	                       CW_USEDEFAULT, CW_USEDEFAULT, // size
						   NULL, NULL, hInstance, NULL );
	return hWnd;
}

int APIENTRY WinMain(HINSTANCE hinst, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/)
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
	}

	g_conf.Initialize(g_config);


	if( !LoadOptions() )
		SetDefaultOptions();

	g_env.nNeedCursor  = 0;
	g_env.minimized    = false;
	g_env.camera_x     = 0;
	g_env.camera_y     = 0;
	OPT(players)[MAX_HUMANS].bAI = TRUE;

	GC_Sound::_countMax = g_conf.s_maxchanels->GetInt();


	//
	// init editor
	//

//	g_editor = new Editor();




	//
	// init common controls
	//

	TRACE("windows common controls initialization\n");
	INITCOMMONCONTROLSEX iccex = {
	sizeof(INITCOMMONCONTROLSEX),
		0
//		ICC_LISTVIEW_CLASSES|ICC_UPDOWN_CLASS|ICC_BAR_CLASSES
	};
	InitCommonControlsEx(&iccex);


	//
	// create main app window
	//
	MyRegisterClass(hinst);
	g_env.hMainWnd = CreateMainWnd(hinst);




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
		g_gui->Resize((float) g_render->getXsize(), (float) g_render->getYsize());


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
				if( g_level && g_env.envInputs.keys[DIK_SPACE] && g_level->_modeEditor )
				{
					DialogBox(g_hInstance, (LPCTSTR)IDD_SELECT_OBJECT, g_env.hMainWnd, (DLGPROC) dlgSelectObject);
					continue;
				}
				else if( g_level && g_env.envInputs.keys[DIK_F8] && g_level->_modeEditor )
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
		catch(...)	// general error handling
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


	TRACE("Saving options\n");
	SaveOptions();

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

	// editor
//	SAFE_DELETE(g_editor);

	// config
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
