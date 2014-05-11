// globals.h

#pragma once

#include <memory>

struct IRender;
class TextureManager;
class World;
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

extern unsigned int g_sounds[SND_COUNT];

extern std::unique_ptr<IRender> g_render;
extern TextureManager  *g_texman;
extern UI::LayoutManager  *g_gui;
extern AppBase         *g_app;
//extern ClientBase      *g_client;

extern World     *g_level;
#ifndef NOSOUND
class MusicPlayer;
extern std::unique_ptr<MusicPlayer>     g_music;
#endif
extern std::shared_ptr<FS::FileSystem>  g_fs;

struct GLFWwindow;
extern GLFWwindow *g_appWindow;


///////////////////////////////////////////////////////////////////////////////


struct lua_State;

struct ENVIRONMENT
{
	lua_State *L;
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
