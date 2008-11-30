// globals.h

#pragma once

// forward declarations
struct IRender;

class TextureManager;
class CSoundManager;
class CSound;
class MusicPlayer;
class GuiManager;
class Level;
class IFileSystem;
class ConsoleBuffer;
class KeyMapper;
class TankServer;
class TankClient;

struct GAMEOPTIONS;
struct ENVIRONMENT;

#include "SoundTemplates.h" // FIXME!
#include "struct.h"         // FIXME!

// ------------------------

extern CSound *g_pSounds[SND_COUNT];

extern HINSTANCE    g_hInstance;
extern LPDIRECTINPUTDEVICE8 g_pKeyboard;

extern IRender         *g_render;
extern TextureManager  *g_texman;
extern CSoundManager   *g_soundManager;
extern MusicPlayer     *g_music;
extern GuiManager      *g_gui;
extern SafePtr<Level>   g_level;
extern TankServer      *g_server;
extern TankClient      *g_client;
extern ConsoleBuffer   *g_console;
extern KeyMapper       *g_keys;
extern SafePtr<IFileSystem> g_fs;


///////////////////////////////////////////////////////////////////////////////

struct InputState
{
	bool  keys[300];
	int   mouse_x;
	int   mouse_y;
	int   mouse_wheel;
	bool  bLButtonState;
	bool  bRButtonState;
	bool  bMButtonState;
};


struct ENVIRONMENT
{
	InputState envInputs;

	lua_State *L;            // handle to the script engine

	int       pause;

	int       nNeedCursor;   // счетчик контроллеров, которым нужен курсор
	bool      minimized;     // признак свернутости главного окна.

	int       camera_x;
	int       camera_y;

	HWND      hMainWnd;      // handle to main application window
};

extern ENVIRONMENT g_env;

// end of file
