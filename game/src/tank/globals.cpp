// globals.cpp

#include "stdafx.h"
#include "globals.h"

#include "fs/FileSystem.h"
#include "Level.h"

MD5 g_md5;

ENVIRONMENT g_env;

CSoundManager   *g_soundManager;
CSound          *g_pSounds[SND_COUNT];

AppBase     *g_app;
std::unique_ptr<IRender> g_render;
UI::LayoutManager  *g_gui;

TextureManager *g_texman;
//ClientBase     *g_client;

std::unique_ptr<Level>   g_level;
#ifndef NOSOUND
#include "sound/MusicPlayer.h"
SafePtr<MusicPlayer>     g_music;
#endif
SafePtr<FS::FileSystem>  g_fs;

GLFWwindow *g_appWindow;


UI::ConsoleBuffer& GetConsole()
{
	static UI::ConsoleBuffer buf(100, 500);
	return buf;
}

// end of file
