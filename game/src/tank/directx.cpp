// directx.cpp
//------------------------------------------------------------------------

#include "stdafx.h"

#include "directx.h"

#include "Macros.h"

#include "core/Debug.h"
#include "core/Console.h"

#include "video/TextureManager.h"

#include "config/Config.h"

#include "sound/sfx.h"
#include "sound/MusicPlayer.h"

//--------------------------------------------------------------------------

LPDIRECTINPUT8        g_pDI       = NULL;
//LPDIRECTINPUTDEVICE8  g_pMouse    = NULL;

//--------------------------------------------------------------------------

VOID LoadSurfaces()
{
	if( g_texman->LoadPackage(FILE_TEXTURES) <= 0 )
	{
		TRACE("WARNING: no textures loaded\n");
		MessageBox(g_env.hMainWnd, "ой! а что с текстурами?", TXT_VERSION, MB_ICONERROR);
	}

	if( g_texman->LoadDirectory("skins", "skin/") <= 0 )
	{
		TRACE("WARNING: no skins found\n");
		MessageBox(g_env.hMainWnd, "ой! а где скины?", TXT_VERSION, MB_ICONERROR);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

#if !defined NOSOUND


//-------------------------------------------------------//
// throw LoadSoundException
//-------------------------------------------------------//
HRESULT InitDirectSound(HWND hWnd, bool init)
{
	HRESULT hr = S_OK;

	if( init )
	{
		_ASSERT(!g_soundManager);
		TRACE("Init direct sound...\n");
		g_soundManager = new CSoundManager();
		if( FAILED(hr = g_soundManager->Initialize(hWnd, DSSCL_PRIORITY, 2, 44100, 16)) )
		{
			if( FAILED(hr = g_soundManager->Initialize(hWnd, DSSCL_NORMAL, 2, 44100, 16)) )
			{
				TRACE("ERROR: direct sound init failed\n");
				SAFE_DELETE(g_soundManager);
			}
		}
	}
	else
	{
		TRACE("free direct sound...\n");
	}

	if( !g_soundManager ) return hr;


	try
	{
	LoadOggVorbis(init, SND_BoomStandard,   "sounds\\explosions\\standard.ogg"    );
	LoadOggVorbis(init, SND_BoomBig,        "sounds\\explosions\\big.ogg"         );
	LoadOggVorbis(init, SND_WallDestroy,    "sounds\\explosions\\wall.ogg"        );

	LoadOggVorbis(init, SND_Hit1,           "sounds\\projectiles\\hit1.ogg"       );
	LoadOggVorbis(init, SND_Hit3,           "sounds\\projectiles\\hit2.ogg"       );
	LoadOggVorbis(init, SND_Hit5,           "sounds\\projectiles\\hit3.ogg"       );
	LoadOggVorbis(init, SND_AC_Hit1,        "sounds\\projectiles\\ac_hit_1.ogg"   );
	LoadOggVorbis(init, SND_AC_Hit2,        "sounds\\projectiles\\ac_hit_2.ogg"   );
	LoadOggVorbis(init, SND_AC_Hit3,        "sounds\\projectiles\\ac_hit_3.ogg"   );
	LoadOggVorbis(init, SND_RocketFly,      "sounds\\projectiles\\rocketfly.ogg"  ); //
	LoadOggVorbis(init, SND_DiskHit,        "sounds\\projectiles\\DiskHit.ogg"    ); //
	LoadOggVorbis(init, SND_BfgFlash,       "sounds\\projectiles\\bfgflash.ogg"   ); //
	LoadOggVorbis(init, SND_PlazmaHit,      "sounds\\projectiles\\plazmahit.ogg"  );
	LoadOggVorbis(init, SND_BoomBullet,     "sounds\\projectiles\\bullet.ogg"     ); //

	LoadOggVorbis(init, SND_TargetLock,     "sounds\\turrets\\activate.ogg"       );
	LoadOggVorbis(init, SND_TuretRotate,    "sounds\\turrets\\rotate.ogg"         );
	LoadOggVorbis(init, SND_TuretWakeUp,    "sounds\\turrets\\arming.ogg"         );
	LoadOggVorbis(init, SND_TuretWakeDown,  "sounds\\turrets\\unarming.ogg"       );

	LoadOggVorbis(init, SND_RocketShoot,    "sounds\\pickup\\rocketshoot.ogg"     ); //
	LoadOggVorbis(init, SND_Shoot,          "sounds\\pickup\\shoot.ogg"           ); //
	LoadOggVorbis(init, SND_MinigunFire,    "sounds\\pickup\\MinigunFire.ogg"     );
	LoadOggVorbis(init, SND_WeapReload,     "sounds\\pickup\\reload.ogg"          );
	LoadOggVorbis(init, SND_ACShoot,        "sounds\\pickup\\ac_shoot.ogg"        );
	LoadOggVorbis(init, SND_AC_Reload,      "sounds\\pickup\\ac_reload.ogg"       );
	LoadOggVorbis(init, SND_Pickup,         "sounds\\pickup\\pickup.ogg"          );
	LoadOggVorbis(init, SND_B_Start,        "sounds\\pickup\\b_start.ogg"         );
	LoadOggVorbis(init, SND_B_Loop,         "sounds\\pickup\\b_loop.ogg"          );
	LoadOggVorbis(init, SND_B_End,          "sounds\\pickup\\b_end.ogg"           );
	LoadOggVorbis(init, SND_w_Pickup,       "sounds\\pickup\\w_pickup.ogg"        ); //
	LoadOggVorbis(init, SND_Bolt,           "sounds\\pickup\\boltshoot.ogg"       );
	LoadOggVorbis(init, SND_DiskFire,       "sounds\\pickup\\ripper.ogg"          ); //
	LoadOggVorbis(init, SND_puRespawn,      "sounds\\pickup\\puRespawn.ogg"       );
	LoadOggVorbis(init, SND_TowerRotate,    "sounds\\pickup\\tower_rotate.ogg"    );
	LoadOggVorbis(init, SND_ShockActivate,  "sounds\\pickup\\shockactivate.ogg"   ); //
	LoadOggVorbis(init, SND_BfgInit,        "sounds\\pickup\\bfginit.ogg"         );
	LoadOggVorbis(init, SND_BfgFire,        "sounds\\pickup\\bfgfire.ogg"         );
	LoadOggVorbis(init, SND_PlazmaFire,     "sounds\\pickup\\plazma1.ogg"         );
	LoadOggVorbis(init, SND_RamEngine,      "sounds\\pickup\\ram_engine.ogg"      ); //
	LoadOggVorbis(init, SND_InvEnd,         "sounds\\pickup\\inv_end.ogg"         );
	LoadOggVorbis(init, SND_Inv,            "sounds\\pickup\\inv.ogg"             );
	LoadOggVorbis(init, SND_InvHit1,        "sounds\\pickup\\inv_hit1.ogg"        );
	LoadOggVorbis(init, SND_InvHit2,        "sounds\\pickup\\inv_hit2.ogg"        );

	LoadOggVorbis(init, SND_Impact1,        "sounds\\vehicle\\impact1.ogg"        );
	LoadOggVorbis(init, SND_Impact2,        "sounds\\vehicle\\impact2.ogg"        );
	LoadOggVorbis(init, SND_Slide1,         "sounds\\vehicle\\slide1.ogg"         );
	LoadOggVorbis(init, SND_TankMove,       "sounds\\vehicle\\tank_move.ogg"      );

	LoadOggVorbis(init, SND_Screenshot,     "sounds\\misc\\screenshot.ogg"      ); //
	LoadOggVorbis(init, SND_Limit,          "sounds\\misc\\limit.ogg"           );
	LoadOggVorbis(init, SND_LightSwitch,    "sounds\\misc\\light1.ogg"          ); //
	}
	catch( LoadSoundException )
	{
		if( init ) FreeDirectSound();
		throw;
	}

	return S_OK;
}

void FreeDirectSound()
{
	InitDirectSound(NULL, 0);
	SAFE_DELETE(g_music);
	SAFE_DELETE(g_soundManager);
}

#endif

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

HRESULT InitDirectInput( HWND hWnd )
{
	TRACE("init direct input\n");

	ZeroMemory(g_env.envInputs.keys, sizeof(g_env.envInputs.keys));
    FreeDirectInput();

	DWORD dwPriority = DISCL_NONEXCLUSIVE | DISCL_FOREGROUND;


    HRESULT hr;

    if( FAILED( hr = DirectInput8Create( GetModuleHandle(NULL), DIRECTINPUT_VERSION,
                                         IID_IDirectInput8, (VOID**)&g_pDI, NULL ) ) )
        return hr;
    if( FAILED( hr = g_pDI->CreateDevice( GUID_SysKeyboard, &g_pKeyboard, NULL ) ) )
        return hr;
    if( FAILED( hr = g_pKeyboard->SetDataFormat( &c_dfDIKeyboard ) ) )
        return hr;
    if( FAILED(hr = g_pKeyboard->SetCooperativeLevel( hWnd, dwPriority)) )
        return hr;
    g_pKeyboard->Acquire();



//    if( FAILED( hr = g_pDI->CreateDevice( GUID_SysMouse, &g_pMouse, NULL ) ) )
//        return hr;
//    if( FAILED( hr = g_pMouse->SetDataFormat( &c_dfDIMouse2 ) ) )
//        return hr;
//    if( FAILED(hr = g_pMouse->SetCooperativeLevel( hWnd, dwPriority)) )
//        return hr;
//    g_pMouse->Acquire();

	g_env.envInputs.mouse_x = g_render->GetWidth() / 2;
	g_env.envInputs.mouse_y = g_render->GetHeight() / 2;
	g_env.envInputs.mouse_wheel = 0;
	g_env.envInputs.bLButtonState = false;
	g_env.envInputs.bRButtonState = false;
	g_env.envInputs.bMButtonState = false;

    return S_OK;
}

void FreeDirectInput()
{
//    if( g_pMouse )
//        g_pMouse->Unacquire();

    if( g_pKeyboard )
        g_pKeyboard->Unacquire();

//    SAFE_RELEASE( g_pMouse );
    SAFE_RELEASE( g_pKeyboard );
    SAFE_RELEASE( g_pDI );

	TRACE("free direct input\n");
}


HRESULT ReadImmediateData()
{
	HRESULT hr;

	if( NULL == g_pKeyboard )
		return S_OK;

	char data[256];
	hr = g_pKeyboard->GetDeviceState(sizeof(data), data);
	if( FAILED(hr) )
	{
		hr = g_pKeyboard->Acquire();
		while( hr == DIERR_INPUTLOST )
		{
			Sleep(50);
			hr = g_pKeyboard->Acquire();
		}
		return S_OK;
	}

	ZeroMemory(g_env.envInputs.keys, sizeof(g_env.envInputs.keys));
	for( int i = 0; i < sizeof(data); ++i )
	{
		g_env.envInputs.keys[i] = (data[i] & 0x80) != 0;
	}


/*
    DIMOUSESTATE2 dims2 = {0};

    hr = g_pMouse->GetDeviceState( sizeof(DIMOUSESTATE2), &dims2 );
    if( FAILED(hr) )
    {
        hr = g_pMouse->Acquire();
        while( hr == DIERR_INPUTLOST )
		{
			Sleep(50);
            hr = g_pMouse->Acquire();
		}
        return S_OK;
    }

	g_env.envInputs.mouse_wheel = dims2.lZ;

	g_env.envInputs.mouse_x += dims2.lX;
	g_env.envInputs.mouse_y += dims2.lY;
	g_env.envInputs.mouse_x = __max(0,
		__min(g_render->GetWidth() - 1, g_env.envInputs.mouse_x));
	g_env.envInputs.mouse_y = __max(0,
		__min(g_render->GetHeight() - 1, g_env.envInputs.mouse_y));


	g_env.envInputs.bLButtonState = (dims2.rgbButtons[0] & 0x80) != 0;
	g_env.envInputs.bRButtonState = (dims2.rgbButtons[1] & 0x80) != 0;
	g_env.envInputs.bMButtonState = (dims2.rgbButtons[2] & 0x80) != 0;
*/

    return S_OK;
}


//--------------------------------------------------------------------------

HRESULT InitAll( HWND hWnd )
{
	HRESULT  hr;


	_ASSERT(g_render);

	DisplayMode dm;
	dm.Width         = g_conf.r_width->GetInt();
    dm.Height        = g_conf.r_height->GetInt();
    dm.RefreshRate   = g_conf.r_freq->GetInt();
    dm.BitsPerPixel  = g_conf.r_bpp->GetInt();

	if( !g_render->Init(hWnd, &dm, g_conf.r_fullscreen->Get()) )
	{
		return E_FAIL;
	}

	if( FAILED(hr = InitDirectInput(hWnd)) )
	{
		MessageBox( hWnd, "Ошибка инициализации DirectInput", TXT_VERSION, MB_ICONERROR | MB_OK );
		return hr;
	}

#if !defined NOSOUND
	try
	{
		if( FAILED(hr = InitDirectSound(hWnd, true)) )
		{
			MessageBox( hWnd, "Ошибка инициализации звука", TXT_VERSION, MB_ICONERROR | MB_OK );
		}
	}
	catch(LoadSoundException e)
	{
		std::ostringstream ss;
		ss << "Ошибка при загрузке файла " << e.filename;
		MessageBox( hWnd, ss.str().c_str(), TXT_VERSION, MB_ICONERROR | MB_OK );
		return e.hr;
	}
#endif

	return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
// end of file
