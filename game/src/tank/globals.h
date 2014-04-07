// globals.h

#pragma once

#include "core/SafePtr.h"

#include <memory>

struct IRender;
class TextureManager;
class CSoundManager;
class CSound;
class Level;
class ConsoleBuffer;
//class ClientBase;
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

extern std::unique_ptr<IRender> g_render;
extern TextureManager  *g_texman;
extern CSoundManager   *g_soundManager;
extern UI::LayoutManager  *g_gui;
extern AppBase         *g_app;
//extern ClientBase      *g_client;

extern std::unique_ptr<Level>     g_level;
#ifndef NOSOUND
class MusicPlayer;
extern SafePtr<MusicPlayer>     g_music;
#endif
extern SafePtr<FS::FileSystem>  g_fs;

struct GLFWwindow;
extern GLFWwindow *g_appWindow;


///////////////////////////////////////////////////////////////////////////////


struct lua_State;

struct ENVIRONMENT
{
	lua_State *L;

	int       pause;

	int       nNeedCursor;   // number of systems which need the mouse cursor to be visible
	bool      minimized;     // indicates that the main app window is minimized
};

extern ENVIRONMENT g_env;

///////////////////////////////////////////////////////////////////////////////

struct MD5
{
	unsigned char bytes[16];
};

extern MD5 g_md5; // md5 digest of the main executable


// end of file
