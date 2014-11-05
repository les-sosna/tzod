// globals.cpp

#include "globals.h"
#include <ui/ConsoleBuffer.h>

ENVIRONMENT g_env;

UI::ConsoleBuffer& GetConsole()
{
	static UI::ConsoleBuffer buf(100, 500);
	return buf;
}

// end of file
