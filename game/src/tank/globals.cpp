// globals.cpp

#include "globals.h"
#include <ui/ConsoleBuffer.h>

ENVIRONMENT g_env;

unsigned int g_sounds[SND_COUNT];

UI::ConsoleBuffer& GetConsole()
{
	static UI::ConsoleBuffer buf(100, 500);
	return buf;
}

// end of file
