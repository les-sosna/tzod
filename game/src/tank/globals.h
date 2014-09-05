// globals.h

#pragma once

#include <memory>

struct IRender;
class ConsoleBuffer;
//class ClientBase;
class AppBase;


struct GAMEOPTIONS;
struct ENVIRONMENT;

#include "SoundTemplates.h" // FIXME!

// ------------------------

extern unsigned int g_sounds[SND_COUNT];

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
