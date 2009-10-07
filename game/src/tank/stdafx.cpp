// stdafx.cpp

#include "stdafx.h"

UI::ConsoleBuffer& GetConsole()
{
	static UI::ConsoleBuffer buf(100, 500);
	return buf;
}

///////////////////////////////////////////////////////////////////////////////
// end of file
