// directx.cpp
// работа с интерфейсами DirectX
//------------------------------------------------------------------------

#include "stdafx.h"

#include "directx.h"

#include "core/Debug.h"
#include "core/Console.h"

#include "video/TextureManager.h"

#include "Options.h"

//--------------------------------------------------------------------------

LPDIRECTINPUT8        g_pDI       = NULL;
//LPDIRECTINPUTDEVICE8  g_pMouse	  = NULL;

//--------------------------------------------------------------------------

#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

//--------------------------------------------------------------------------

VOID LoadSurfaces()
{
	if( g_texman->LoadPackage(FILE_TEXTURES) <= 0 )
	{
		TRACE("WARNING: no textures loaded\n");
		MessageBox(g_env.hMainWnd, "ой! а что с текстурами?", TXT_VERSION, MB_ICONERROR);
	}


//	LOGOUT_1("> loading skins... \n");
	if( g_texman->LoadDirectory("skins", "skin/") <= 0 )
	{
//		LOGOUT_1("failed \n");
		MessageBox(g_env.hMainWnd, "ой! а где скины?", TXT_VERSION, MB_ICONERROR);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

#if !defined NOSOUND

struct LoadSoundException
{
	string_t  filename;
	HRESULT      hr;
};

void LoadSound(bool init, enumSoundTemplate sound, const char *filename)
{
	if( init )
	{
		HRESULT hr;
		TRACE("loading sound from '%s'...\n", filename);
		if( FAILED(hr = g_pSoundManager->Create(
			&g_pSounds[sound],
			filename,
			DSBCAPS_CTRLPAN|DSBCAPS_CTRLVOLUME|DSBCAPS_CTRLFREQUENCY,
			GUID_NULL)) )
		{
			TRACE("ERROR: unknown\n");
			//-------------------------------------------------------
			LoadSoundException e;
			e.filename = filename;
			e.hr       = hr;
			throw e;
		}
	}
	else
	{
		SAFE_DELETE(g_pSounds[sound]);
	}
}

void LoadOggVorbis(bool init, enumSoundTemplate sound, const char *filename)
{
	if( init )
	{
		TRACE("loading sound from '%s'...\n", filename);

		WAVEFORMATEX wfe = {0};
		void *pData = NULL;
		int size    = 0;

		if( 0 != ogg_load_vorbis(filename, &wfe, &pData, &size) )
		{
			TRACE("ERROR: couldn't load file\n");
			//-------------------------------------------------------
			LoadSoundException e;
			e.filename = filename;
			e.hr       = E_FAIL;
			throw e;
		}

		HRESULT hr = g_pSoundManager->CreateFromMemory( &g_pSounds[sound],
			(BYTE *) pData, size, &wfe,
			DSBCAPS_CTRLPAN|DSBCAPS_CTRLVOLUME|DSBCAPS_CTRLFREQUENCY, GUID_NULL);

		if( FAILED(hr) )
		{
			TRACE("ERROR: couldn't create the sound buffer\n");
			//-------------------------------------------------------
			LoadSoundException e;
			e.filename = filename;
			e.hr       = hr;
			throw e;
		}
	}
	else
	{
		SAFE_DELETE(g_pSounds[sound]);
	}
}

//-------------------------------------------------------//
// throw LoadSoundException
//-------------------------------------------------------//
HRESULT InitDirectSound(HWND hWnd, bool init)
{
	HRESULT hr = S_OK;

	if( init )
	{
		_ASSERT(!g_pSoundManager);
		TRACE("Init direct sound...\n");
		g_pSoundManager = new CSoundManager();
		if( FAILED(hr = g_pSoundManager->Initialize(hWnd, DSSCL_EXCLUSIVE, 2, 44100, 16)) )
		{
			if( FAILED(hr = g_pSoundManager->Initialize(hWnd, DSSCL_PRIORITY , 2, 44100, 16)) )
			{
				if( FAILED(hr = g_pSoundManager->Initialize(hWnd, DSSCL_NORMAL , 2, 44100, 16)) )
				{
					TRACE("ERROR: direct sound init failed\n");
					SAFE_DELETE(g_pSoundManager);
				}
			}
		}
	}
	else
	{
		TRACE("free direct sound...\n");
	}

	if( !g_pSoundManager ) return hr;


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
	LoadOggVorbis(init, SND_PickUp,         "sounds\\pickup\\pickup.ogg"          );
	LoadOggVorbis(init, SND_B_Start,        "sounds\\pickup\\b_start.ogg"         );
	LoadOggVorbis(init, SND_B_Loop,         "sounds\\pickup\\b_loop.ogg"          );
	LoadOggVorbis(init, SND_B_End,          "sounds\\pickup\\b_end.ogg"           );
	LoadOggVorbis(init, SND_w_PickUp,       "sounds\\pickup\\w_pickup.ogg"        ); //
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
	catch(LoadSoundException)
	{
		if(init) FreeDirectSound();
		throw;
	}

	return S_OK;
}


void FreeDirectSound()
{
	InitDirectSound(0, 0);
	SAFE_DELETE(g_pSoundManager);
}

#endif

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

HRESULT InitDirectInput( HWND hWnd )
{
	TRACE("init direct input\n");

	ZeroMemory(g_env.envInputs.keys, 256);
    FreeDirectInput();

//#ifdef _DEBUG
	DWORD dwPriority = DISCL_NONEXCLUSIVE | DISCL_FOREGROUND;
//#else
//	DWORD dwPriority = DISCL_EXCLUSIVE | DISCL_FOREGROUND;
//#endif


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

	g_env.envInputs.mouse_x = g_render->getXsize() / 2;
	g_env.envInputs.mouse_y = g_render->getYsize() / 2;
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

    ZeroMemory(g_env.envInputs.keys, 256);
    hr = g_pKeyboard->GetDeviceState( sizeof(g_env.envInputs.keys[0]) * 256, g_env.envInputs.keys );
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

    for ( int i = 0; i < 256; i++ )
    {
        if( g_env.envInputs.keys[i] & 0x80 )
			g_env.envInputs.keys[i] = 1;
		else
			g_env.envInputs.keys[i] = 0;
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
		__min(g_render->getXsize() - 1, g_env.envInputs.mouse_x));
	g_env.envInputs.mouse_y = __max(0,
		__min(g_render->getYsize() - 1, g_env.envInputs.mouse_y));


	g_env.envInputs.bLButtonState = (dims2.rgbButtons[0] & 0x80) != 0;
	g_env.envInputs.bRButtonState = (dims2.rgbButtons[1] & 0x80) != 0;
	g_env.envInputs.bMButtonState = (dims2.rgbButtons[2] & 0x80) != 0;
*/


    return S_OK;
}


void GetKeyName(int nKey, char *pBuf)
{
	static const char names[][256] = {
		{"<error>"},		// 0
		{"Escape"},			// 1
		{"1"},				// 2
		{"2"},				// 3
		{"3"},				// 4
		{"4"},				// 5
		{"5"},				// 6
		{"6"},				// 7
		{"7"},				// 8
		{"8"},				// 9
		{"9"},				// 10
		{"0"},				// 11
		{"-"},				// 12
		{"="},				// 13
		{"Backspace"},		// 14
		{"Tab"},			// 15
		{"Q"},				// 16
		{"W"},				// 17
		{"E"},				// 18
		{"R"},				// 19
		{"T"},				// 20
		{"Y"},				// 21
		{"U"},				// 22
		{"I"},				// 23
		{"O"},				// 24
		{"P"},				// 25
		{"["},				// 26
		{"]"},				// 27
		{"Enter"},			// 28
		{"Left Control"},	// 29
		{"A"},				// 30
		{"S"},				// 31
		{"D"},				// 32
		{"F"},				// 33
		{"G"},				// 34
		{"H"},				// 35
		{"J"},				// 36
		{"K"},				// 37
		{"L"},				// 38
		{";"},				// 39
		{"'"},				// 40
		{"~"},				// 41
		{"Left Shift"},		// 42
		{"\\"},				// 43
		{"Z"},				// 44
		{"X"},				// 45
		{"C"},				// 46
		{"V"},				// 47
		{"B"},				// 48
		{"N"},				// 49
		{"M"},				// 50
		{","},				// 51
		{"."},				// 52
		{"/"},				// 53
		{"Right Shift"},	// 54
		{"Numpad *"},		// 55
		{"Left Alt"},		// 56
		{"Space"},			// 57
		{"Caps Lock"},		// 38
		{"F1"},				// 39
		{"F2"},				// 60
		{"F3"},				// 61
		{"F4"},				// 62
		{"F5"},				// 63
		{"F6"},				// 64
		{"F7"},				// 65
		{"F8"},				// 66
		{"F9"},				// 67
		{"F10"},			// 68
		{"Num Lock"},		// 69
		{"Scroll Lock"},	// 70
		{"Numpad 7"},		// 71
		{"Numpad 8"},		// 72
		{"Numpad 9"},		// 73
		{"Numpad -"},		// 74
		{"Numpad 4"},		// 75
		{"Numpad 5"},		// 76
		{"Numpad 6"},		// 77
		{"Numpad +"},		// 78
		{"Numpad 1"},		// 79
		{"Numpad 2"},		// 80
		{"Numpad 3"},		// 81
		{"Numpad 0"},		// 82
		{"Numpad ."},		// 83

		{"???-84"},	// 84
		{"???-85"},	// 85
		{"???-86"},	// 86

		{"F11"},	// 87
		{"F12"},	// 88

		{"???-89"},	// 89
		{"???-90"},	// 90
		{"???-91"},	// 91
		{"???-92"},	// 92
		{"???-93"},	// 93
		{"???-94"},	// 94
		{"???-95"},	// 95
		{"???-96"},	// 96
		{"???-97"},	// 97
		{"???-98"},	// 98
		{"???-99"},	// 99
		{"???-100"},	// 100
		{"???-101"},	// 101
		{"???-102"},	// 102
		{"???-103"},	// 103
		{"???-104"},	// 104
		{"???-105"},	// 105
		{"???-106"},	// 106
		{"???-107"},	// 107
		{"???-108"},	// 108
		{"???-109"},	// 109
		{"???-110"},	// 110
		{"???-111"},	// 111
		{"???-112"},	// 112
		{"???-113"},	// 113
		{"???-114"},	// 114
		{"???-115"},	// 115
		{"???-116"},	// 116
		{"???-117"},	// 117
		{"???-118"},	// 118
		{"???-119"},	// 119
		{"???-120"},	// 120
		{"???-121"},	// 121
		{"???-122"},	// 122
		{"???"},	// 123
		{"???"},	// 124
		{"???"},	// 125
		{"???"},	// 126
		{"???"},	// 127
		{"???"},	// 128
		{"???"},	// 129
		{"???"},	// 130
		{"???"},	// 131
		{"???"},	// 132
		{"???"},	// 133
		{"???"},	// 134
		{"???"},	// 135
		{"???"},	// 136
		{"???"},	// 137
		{"???"},	// 138
		{"???"},	// 139
		{"???"},	// 140
		{"???"},	// 141
		{"???"},	// 142
		{"???"},	// 143
		{"???"},	// 144
		{"???"},	// 145
		{"???"},	// 146
		{"???"},	// 147
		{"???"},	// 148
		{"???"},	// 149
		{"???"},	// 150
		{"???"},	// 151
		{"???"},	// 152
		{"???"},	// 153
		{"???"},	// 154
		{"???"},	// 155

		{"Numpad Enter"},	// 156
		{"Right Control"},	// 157

		{"???"},	// 158
		{"???"},	// 159
		{"???"},	// 160
		{"???"},	// 161
		{"???"},	// 162
		{"???"},	// 163
		{"???"},	// 164
		{"???"},	// 165
		{"???"},	// 166
		{"???"},	// 167
		{"???"},	// 168
		{"???"},	// 169
		{"???"},	// 170
		{"???"},	// 171
		{"???"},	// 172
		{"???"},	// 173
		{"???"},	// 174
		{"???"},	// 175
		{"???"},	// 176
		{"???"},	// 177
		{"???"},	// 178
		{"???"},	// 179
		{"???"},	// 180

		{"Numpad /"},	// 181

		{"???"},	// 182

		{"Print Screen"},	// 183
		{"Right Alt"},	// 184

		{"???"},	// 185
		{"???"},	// 186
		{"???"},	// 187
		{"???"},	// 188
		{"???"},	// 189
		{"???"},	// 190
		{"???"},	// 191
		{"???"},	// 192
		{"???"},	// 193
		{"???"},	// 194
		{"???"},	// 195
		{"???"},	// 196

		{"Pause"},			// 197

		{"???"},	// 198

		{"Home"},		// 199
		{"Up Arrow"},	// 200
		{"Page Up"},	// 201

		{"???"},	// 202

		{"Left Arrow"},	// 203

		{"???"},	// 204

		{"Right Arrow"},	// 205

		{"???"},	// 206

		{"End"},		// 207
		{"Down Arrow"},	// 208
		{"Page Down"},	// 209
		{"Insert"},		// 210
		{"Delete"},		// 211

		{"???"},	// 212
		{"???"},	// 213
		{"???"},	// 214
		{"???"},	// 215
		{"???"},	// 216
		{"???"},	// 217
		{"???"},	// 218

		{"Left Win"},	// 219
		{"Right Win"},	// 220
		{"App Menu"},	// 221
		{"System Power"},		// 222
		{"System Sleep"},		// 223

		{"???"},	// 224
		{"???"},	// 225
		{"???"},	// 226
		{"System Wake"},	// 227
		{"???"},	// 228
		{"Web Search"},	// 229
		{"Web Favorites"},	// 230
		{"Web Refresh"},	// 231
		{"Web Stop"},	// 232
		{"Web Forward"},	// 233
		{"Web Back"},	// 234
		{"My Computer"},	// 235
		{"Mail"},	// 236
		{"Media Select"},	// 237
		{"???"},	// 238
		{"???"},	// 239
		{"???"},	// 240
		{"???"},	// 241
		{"???"},	// 242
		{"???"},	// 243
		{"???"},	// 244
		{"???"},	// 245
		{"???"},	// 246
		{"???"},	// 247
		{"???"},	// 248
		{"???"},	// 249
		{"???"},	// 250
		{"???"},	// 251
		{"???"},	// 252
		{"???"},	// 253
		{"???"},	// 254
		{"???"},	// 255
	};

	strcpy(pBuf, names[nKey]);
}


//--------------------------------------------------------------------------

HRESULT InitAll( HWND hWnd )
{
	HRESULT  hr;


	_ASSERT(g_render);

	DisplayMode dm;
    dm.Width         = OPT(dispWidth);
    dm.Height        = OPT(dispHeight);
    dm.RefreshRate   = OPT(dispRefreshRate);
    dm.BitsPerPixel  = OPT(dispBitsPerPixel);
	
	if( !g_render->Init(hWnd, &dm, OPT(bFullScreen)) )
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
		//	return hr;
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
