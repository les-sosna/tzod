// globals.cpp

#include "globals.h"
#include "World.h"
#include <ConsoleBuffer.h>
#include <FileSystem.h>

ENVIRONMENT g_env;

unsigned int     g_sounds[SND_COUNT];

IRender *g_render;

TextureManager *g_texman;
//ClientBase     *g_client;

World   *g_level;
#ifndef NOSOUND
#include "sound/MusicPlayer.h"
std::unique_ptr<MusicPlayer>     g_music;
#endif
std::shared_ptr<FS::FileSystem>  g_fs;

GLFWwindow *g_appWindow;


UI::ConsoleBuffer& GetConsole()
{
	static UI::ConsoleBuffer buf(100, 500);
	return buf;
}

// end of file
