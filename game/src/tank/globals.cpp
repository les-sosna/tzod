// globals.cpp

#include "globals.h"
#include <ConsoleBuffer.h>
#include <FileSystem.h>

ENVIRONMENT g_env;

unsigned int     g_sounds[SND_COUNT];

TextureManager *g_texman;
//ClientBase     *g_client;

GLFWwindow *g_appWindow;


UI::ConsoleBuffer& GetConsole()
{
	static UI::ConsoleBuffer buf(100, 500);
	return buf;
}

// end of file
