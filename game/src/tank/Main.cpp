// Main.cpp

#include "stdafx.h"

#include "macros.h"
#include "Level.h"
#include "options.h"
#include "directx.h"

#include "core/debug.h"

#include "network/datablock.h"
#include "network/TankClient.h"

#include "video/TextureManager.h"
#include "video/RenderBase.h"

#include "ui/Interface.h"
#include "ui/GuiManager.h"
#include "ui/gui.h"
#include "ui/gui_desktop.h"

#include "gc/GameClasses.h"
#include "gc/Sound.h"
#include "gc/Editor.h"
#include "gc/Player.h"
#include "gc/RigidBody.h"
#include "gc/Indicators.h"

#include "fs/FileSystem.h"

#include "res/resource.h"

/////////////////////////////////////////////////////////////

static void OnPrintScreen()
{
	g_level->_timer.Stop();
	PLAY(SND_Screenshot, vec2d(0, 0));

	// определяем № файла

	CreateDirectory(DIR_SCREENSHOTS, NULL);
	SetCurrentDirectory(DIR_SCREENSHOTS);

	int n = OPT(nScreenshotNumber);
	char name[MAX_PATH];
	while (1)
	{
		char zero[4] = {0};
		if (n < 1000) zero[0] = '0';
		if (n <  100) zero[1] = '0';
		if (n <   10) zero[2] = '0';

		wsprintf(name, "screenshot%s%d.tga", zero, n);

		WIN32_FIND_DATA fd = {0};
		HANDLE h = FindFirstFile(name, &fd);

		if( INVALID_HANDLE_VALUE == h )
			break;

		FindClose(h);
		n++;
	}

	OPT(nScreenshotNumber) = n;

	LOGOUT_1("Screenshot ");
	if ( !g_render->TakeScreenshot(name) )
	{
		LOGOUT_1("FAILED!\n");
		_MessageArea::Inst()->message("> ошибка!");
	}
	else
	{
		LOGOUT_2("'%s'\n", name);
	//	_MessageArea::Inst()->message(name);
	}

	SetCurrentDirectory("..");

	g_level->_timer.Start();
}

////////////////////////////////////////////////////////////////////////////////////////

static void TimeStep()
{
	if( g_level->_limitHit ) return;

	float dt = g_level->_timer.GetDt() * (float) OPT(gameSpeed) / 100.0f;
	_ASSERT(dt >= 0);

	if( OPT(bModeEditor) ) return;

	if( g_level->_client )
	{
		//
		// network mode
		//

		float fixed_dt = 1.0f / (float) OPT(serverFps) / NET_MULTIPLER;
		g_level->_timeBuffer += dt;
		if( g_level->_timeBuffer > 0 )
		do
		{
			//
			// обработка команд кадра
			//
			DataBlock db;
			while( g_level->_client->GetData(db) )
			{
				switch( db.type() )
				{
					case DBTYPE_TEXTMESSAGE:
						_MessageArea::Inst()->message( (LPCTSTR) db.data() );
						break;
					case DBTYPE_SERVERQUIT:
						_MessageArea::Inst()->message( "Сервер вышел" );
						break;
					case DBTYPE_PLAYERQUIT:
					{
						const DWORD &id = db.cast<DWORD>();
						OBJECT_LIST::iterator it = g_level->players.begin();
						while( it != g_level->players.end() )
						{
							if( id == ((GC_Player *)(*it))->_networkId )
							{
								(*it)->Kill();
								_MessageArea::Inst()->message( "игрок вышел" );
								break;
							}
							++it;
						}
					} break;
					case DBTYPE_CONTROLPACKET:
					{
						_ASSERT(0 == db.size() % sizeof(ControlPacket));
						size_t count = db.size() / sizeof(ControlPacket);
						for( size_t i = 0; i < count; i++ )
							g_level->_client->_ctrlBuf.push( ((ControlPacket *) db.data())[i] );
					} break;
				} // end of switch( db.type )

				if( DBTYPE_CONTROLPACKET == db.type() ) break;
			}

			if( g_level->_client->_ctrlBuf.empty() )
			{
				g_level->_timeBuffer = 0;
				break;	// нет кадра. пропускаем
			}


			//
			// ок, кадр получен. расчет игровой ситуации
			//

			#ifdef NETWORK_DEBUG
			DWORD dwCheckSum = 0, tmp_cs;
			#endif


			g_level->_time        += fixed_dt;
			g_level->_timeBuffer -= fixed_dt;
			OBJECT_LIST::safe_iterator it = g_level->ts_fixed.safe_begin();
			while( it != g_level->ts_fixed.end() )
			{
				GC_Object* pTS_Obj = *it;
				_ASSERT(!pTS_Obj->IsKilled());

				// расчет контрольной суммы для обнаружения потери синхронизации
				#ifdef NETWORK_DEBUG
				if( tmp_cs = pTS_Obj->checksum() )
				{
					dwCheckSum = dwCheckSum ^ tmp_cs ^ 0xD202EF8D;
                    dwCheckSum = (dwCheckSum >> 1) | ((dwCheckSum & 0x00000001) << 31);
				}
				#endif

				pTS_Obj->TimeStepFixed(fixed_dt);
				++it;
			}

			GC_RigidBodyDynamic::ProcessResponse(fixed_dt);

#ifdef NETWORK_DEBUG
			g_level->_dwChecksum = dwCheckSum;
#endif
		} while(0);
	}
	else
	{
		int count = int(dt / MAX_DT_FIXED) + 1;
		float fixed_dt = dt / (float) count;

		do
		{
			g_level->_time += fixed_dt;
			OBJECT_LIST::safe_iterator it = g_level->ts_fixed.safe_begin();
			while( it != g_level->ts_fixed.end() )
			{
				GC_Object* pTS_Obj = *it;
				_ASSERT(!pTS_Obj->IsKilled());
				pTS_Obj->TimeStepFixed(fixed_dt);
				++it;
			}
			GC_RigidBodyDynamic::ProcessResponse(fixed_dt);
		} while( --count );
	}

	OBJECT_LIST::safe_iterator it = g_level->ts_floating.safe_begin();
	while( it != g_level->ts_floating.end() )
	{
		GC_Object* pTS_Obj = *it;
		_ASSERT(!pTS_Obj->IsKilled());
		pTS_Obj->TimeStepFloat(dt);
		++it;
	}
}

/////////////////////////////////////////////////////////////

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
			ENUM_BEGIN(cameras, GC_Camera, pCamera)
			{
				if( !pCamera->IsActive() ) continue;
				if( pCamera->GetShake() >= max_shake )
				{
					pMaxShake = pCamera;
					max_shake = pCamera->GetShake();
				}
			} ENUM_END();
		}

		int count = 0;
		ENUM_BEGIN(cameras, GC_Camera, pCamera)
		{
			if( pMaxShake ) pCamera = pMaxShake;
			if( !pCamera->IsActive() ) continue;
			pCamera->Select();


			//
			// рендеринг освещения
			//

			if( OPT(bNightMode) )
			{
				g_render->setMode(RM_LIGHT);

				float xmin = (float) __max(0, g_env.camera_x );
				float ymin = (float) __max(0, g_env.camera_y );
				float xmax = __min(g_level->_sx, (float) g_env.camera_x +
					(float) g_render->getViewportXsize() / pCamera->_zoom );
				float ymax = __min(g_level->_sy, (float) g_env.camera_y +
					(float) g_render->getViewportYsize() / pCamera->_zoom );

				ENUM_BEGIN(lights, GC_Light, pLight)
				{
					_ASSERT(!pLight->IsKilled());
					if( pLight->_pos.x + pLight->GetRenderRadius() > xmin &&
						pLight->_pos.x - pLight->GetRenderRadius() < xmax &&
						pLight->_pos.y + pLight->GetRenderRadius() > ymin &&
						pLight->_pos.y - pLight->GetRenderRadius() < ymax )
					{
						pLight->Shine();
					}
				} ENUM_END();
			}

		//	printf("cx=%d; cy=%d\n", g_env.camera_x, g_env.camera_y);


			//
			// paint all objects
			//

			g_render->setMode(RM_WORLD);

			// paint background texture
			_Background::Inst()->Draw();

			for( int z = 0; z < Z_COUNT-1; ++z )
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
						ENUM_BEGIN(z_grids[z](lev).element(x,y), GC_2dSprite, object)
						{
							_ASSERT(!object->IsKilled());
							object->Draw();
							_ASSERT(!object->IsKilled());
						} ENUM_END();
					}
				} // loop over gridsets


				// loop over globals
				ENUM_BEGIN(z_globals[z], GC_2dSprite, object)
				{
					_ASSERT(!object->IsKilled());
					object->Draw();
					_ASSERT(!object->IsKilled());
				} ENUM_END();
			}

			if( pMaxShake ) break;
		} ENUM_END();	// cameras


		//
		// paint Z_SCREEN layer
		//

		if( !thumbnail )
		{
			g_render->setMode(RM_INTERFACE);

			ENUM_BEGIN(z_globals[Z_COUNT-1], GC_2dSprite, object)
			{
				_ASSERT(!object->IsKilled());
				object->Draw();
				_ASSERT(!object->IsKilled());
			} ENUM_END();
		}
	}

	if( g_gui ) g_gui->Draw();


	// display new frame
	g_render->End();


	// check for print screen key is pressed
	static char _oldRQ = 0;
	if( g_env.envInputs.keys[DIK_SYSRQ] && !_oldRQ )
		OnPrintScreen();
	_oldRQ = g_env.envInputs.keys[DIK_SYSRQ];
}

static void EndFrame()
{
	OBJECT_LIST::safe_iterator it = g_level->endframe.safe_begin();
	while( it != g_level->endframe.end() )
	{
		GC_Object* pEF_Obj = *it;
		pEF_Obj->EndFrame();
		++it;
	}
}

///////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
// maximum mumber of lines the output console should have
static const WORD MAX_CONSOLE_LINES = 500;

static BOOL RedirectIOToConsole()
{
	int hConHandle;
	long lStdHandle;
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	FILE *fp;

	// allocate a console for this app
	AllocConsole();

	// set the screen buffer to be big enough to let us scroll text
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),&coninfo);
	coninfo.dwSize.Y = MAX_CONSOLE_LINES;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE),coninfo.dwSize);

	// redirect unbuffered STDOUT to the console
	lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "w" );
	*stdout = *fp;
	setvbuf( stdout, NULL, _IONBF, 0 );

	// redirect unbuffered STDIN to the console
	lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "r" );
	*stdin = *fp;
	setvbuf( stdin, NULL, _IONBF, 0 );

	// redirect unbuffered STDERR to the console
	lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen( hConHandle, "w" );
	*stderr = *fp;
	setvbuf( stderr, NULL, _IONBF, 0 );

	// make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
	// point to console as well
	std::ios::sync_with_stdio();

	return TRUE;
}
#endif // ifdef _DEBUG

/////////////////////////////////////////////////////////////

static UI::Window* CreateDesktopWindow(GuiManager *mgr)
{
	return new UI::Desktop(mgr);
}


static HWND CreateMainWnd(HINSTANCE hInstance, int width, int height)
{
	LOGOUT_1("create main window\n");
    HWND hWnd = CreateWindowEx( 0, TXT_WNDCLASS, TXT_VERSION,
                           WS_POPUP|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_MAXIMIZEBOX|WS_SYSMENU,
						   CW_USEDEFAULT, CW_USEDEFAULT, // position
  	                       CW_USEDEFAULT, CW_USEDEFAULT, // size
						   NULL, NULL, hInstance, NULL );
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
	return hWnd;
}



int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	_ASSERT(RedirectIOToConsole());

	g_hInstance = hInstance;

	//
	// init log file
	//
	InitLogFile(FILE_LOG);
	LOGOUT_1("--- Initialize ---\n");


	//
	// init file system
	//

	g_fs = OSFileSystem::Create(".");



	//
	// init options
	//
	
	if( !LoadOptions() ) 
		SetDefaultOptions();

	g_env.nNeedCursor  = 0;
	g_env.bMinimized   = false;
	g_env.camera_x     = 0;
	g_env.camera_y     = 0;
	OPT(bModeEditor)   = false;
	OPT(gameType)      = GT_EDITOR;
	OPT(players)[MAX_HUMANS].bAI = TRUE;

	GC_Sound::_countMax = OPT(nSoundChannels);



	//
	// show graphics mode selection dialog
	//
	if( OPT(bShowSelectMode) && IDOK != DialogBox(g_hInstance,
		(LPCTSTR) IDD_DISPLAY, NULL, (DLGPROC) dlgDisplaySettings) )
	{
		g_fs = NULL; // free the file system
		return 0;
	}



	//
	// init scripting system
	//

	LOGOUT_1("script engine initialization... ");
	if( NULL == (g_env.hScript = script_open()) )
	{
		LOGOUT_1("FAILED\n");
		return -1;
	}
	LOGOUT_1("ok\n");


	//
	// init common controls
	//

	LOGOUT_1("init common controls\n");
	INITCOMMONCONTROLSEX iccex = {
	sizeof(INITCOMMONCONTROLSEX),
		0
//		ICC_LISTVIEW_CLASSES|ICC_UPDOWN_CLASS|ICC_BAR_CLASSES 
	};
	InitCommonControlsEx(&iccex);




#ifndef _DEBUG
	BOOL bGeneralFault = FALSE;
#endif

	MyRegisterClass(hInstance);
	g_env.hMainWnd = CreateMainWnd(hInstance, g_render->getXsize(), g_render->getYsize());


	if( SUCCEEDED(InitAll(g_env.hMainWnd)) )
	{
		// init texture manager
		g_texman = new TextureManager;

		LoadSurfaces();

		LOGOUT_1("--- Begin main loop ---\n");
		timeBeginPeriod(1);

#ifndef _DEBUG
		try
		{
		;
#endif
		// init GUI
		g_gui = new GuiManager(CreateDesktopWindow);
		g_render->OnResizeWnd();
		g_gui->Resize((float) g_render->getXsize(), (float) g_render->getYsize());


		if( !script_exec_file(g_env.hScript, FILE_STARTUP) )
		{
			MessageBoxT(g_env.hMainWnd, "startup script error", MB_ICONERROR);
		}


		MSG msg;
		while(true) // цикл обработки сообщений
		{
			if( PeekMessage(&msg, NULL, 0, 0, TRUE) )
			{
				if( WM_QUIT == msg.message ) break;

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else if( g_level && (!g_env.bMinimized || g_level->_client) )
			{
				ReadImmediateData();  // чтение состояния устройств ввода
				//------------------------------
			/*	if (g_env.envInputs.keys[DIK_ESCAPE])
				{
					LOGOUT_1("DialogBox(IDD_MAIN)\n");
					g_level->Pause(true);
					DialogBoxParam(g_hInstance, (LPCTSTR)IDD_MAIN, g_env.hMainWnd, (DLGPROC) dlgMain, (LPARAM) g_env.hMainWnd);
					if (g_level) g_level->Pause(false);
					continue;
				}
				else*/ if( g_env.envInputs.keys[DIK_SPACE] && g_options.bModeEditor )
				{
					LOGOUT_1("DialogBox(IDD_SELECT_OBJECT)\n");
					DialogBox(g_hInstance, (LPCTSTR)IDD_SELECT_OBJECT, g_env.hMainWnd, (DLGPROC) dlgSelectObject);
					continue;
				}
				else if( g_env.envInputs.keys[DIK_RETURN] && OPT(bModeEditor) && _Editor::Inst()->GetSelection() )
				{
					IPropertySet *p = _Editor::Inst()->GetSelection()->GetProperties();
					if( NULL != p )
					{
						LOGOUT_1("DialogBox(IDD_OBJPROP)\n");
						DialogBoxParam(g_hInstance, (LPCTSTR)IDD_OBJPROP, g_env.hMainWnd, (DLGPROC) dlgObjectProperties, (LPARAM) p);
						p->Release();
					}
					continue;
				}
				else if( g_env.envInputs.keys[DIK_F8] && g_options.bModeEditor )
				{
					LOGOUT_1("DialogBox(IDD_MAP_INFO)\n");
					DialogBox(g_hInstance, (LPCTSTR)IDD_MAP_SETTINGS, g_env.hMainWnd, (DLGPROC) dlgMapSettings);
					continue;
				}
				else if( g_env.envInputs.keys[DIK_LALT] && g_env.envInputs.keys[DIK_TAB] )
				{
					g_level->Pause(true);
					ShowWindow(g_env.hMainWnd, SW_MINIMIZE);
					g_level->Pause(false);
					continue;
				}
//				else if( g_env.envInputs.keys[DIK_F2] )
//				{
//					LOGOUT_1("DialogBox(IDD_NEWDM)\n");
//					g_level->Pause(true);
//					DialogBox(g_hInstance, (LPCTSTR)IDD_NEWDM, g_env.hMainWnd, (DLGPROC) dlgNewDM);
//					g_level->Pause(false);
//					continue;
//				}
				else if( g_env.envInputs.keys[DIK_F12] )
				{
					LOGOUT_1("DialogBox(IDD_OPTIONS)\n");
					g_level->Pause(true);
					DialogBox(g_hInstance, (LPCTSTR)IDD_OPTIONS, g_env.hMainWnd, (DLGPROC) dlgOptions);
					g_level->Pause(false);
					continue;
				}
				else if( g_env.envInputs.keys[DIK_LALT] && g_env.envInputs.keys[DIK_F4] )
				{
					LOGOUT_1("Alt + F4 has been pressed. DestroyWindow\n");
					DestroyWindow(g_env.hMainWnd);
					continue;
				}
				//--------------------------------
				TimeStep();
				RenderFrame(false);
				EndFrame();
			}
			else // для снятия нагрузки с процессора, когда игра не активна
			{
				GetMessage(&msg, NULL, 0, 0);
				if( msg.message == WM_QUIT ) break;

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

#ifndef _DEBUG
		} // end of try block
		catch(...)	// general error handling
		{
			LOGOUT_1("GENERAL FAULT\n");
			bGeneralFault = TRUE;
		}
#endif
		SAFE_DELETE(g_gui);
		timeEndPeriod(1);
	} // end if ( SUCCEEDED(InitAll(hWnd)) )
	else
	{
		MessageBoxT(NULL, "Ошибка инициализации", MB_ICONERROR);
	}

	LOGOUT_1("--- Shotdown ---\n");

	SaveOptions();
	FreeDirectInput();

	g_texman->UnloadAllTextures();
	SAFE_DELETE(g_texman);

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
	LOGOUT_1("script engine shutdown... ");
	script_close(g_env.hScript);
	g_env.hScript = NULL;
	LOGOUT_1("ok\n");

	// clean up the file system
	g_fs = NULL;


	Sleep(500);
	LOGOUT_1("\n--- exit ---\n");
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// end of file
