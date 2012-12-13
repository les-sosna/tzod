// globals.h

#pragma once

// forward declarations
struct IRender;

class TextureManager;
class CSoundManager;
class CSound;
class MusicPlayer;
class Level;
class ConsoleBuffer;
class ClientBase;
class AppBase;

namespace UI
{
	class LayoutManager;
}

namespace FS
{
	class FileSystem;
}

struct GAMEOPTIONS;
struct ENVIRONMENT;

#include "SoundTemplates.h" // FIXME!

// ------------------------

extern CSound *g_pSounds[SND_COUNT];

extern IRender         *g_render;
extern TextureManager  *g_texman;
extern CSoundManager   *g_soundManager;
extern UI::LayoutManager  *g_gui;
extern AppBase         *g_app;
extern ClientBase      *g_client;

extern std::unique_ptr<Level>     g_level;
extern SafePtr<MusicPlayer>     g_music;
extern SafePtr<FS::FileSystem>  g_fs;


///////////////////////////////////////////////////////////////////////////////

struct InputState
{
	bool  _keys[300];
	int   mouse_x;
	int   mouse_y;
	int   mouse_wheel;
	bool  bLButtonState;
	bool  bRButtonState;
	bool  bMButtonState;
	bool IsKeyPressed(int code)
	{
		if( code > 0 && code < sizeof(_keys) / sizeof(_keys[0]) )
		{
			return _keys[code];
		}
		return false;
	}
};


struct ENVIRONMENT
{
	InputState envInputs;

	lua_State *L;            // handle to the script engine

	int       pause;

	int       nNeedCursor;   // number of systems which need the mouse cursor to be visible
	bool      minimized;     // indicates that the main app window is minimized

	HWND      hMainWnd;      // handle to main application window
};

extern ENVIRONMENT g_env;

///////////////////////////////////////////////////////////////////////////////

struct MD5
{
	unsigned char bytes[16];
};

extern MD5 g_md5; // md5 digest of the main executable


// end of file
