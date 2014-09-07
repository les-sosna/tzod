// globals.h

#pragma once

#include "SoundTemplates.h" // FIXME!
extern unsigned int g_sounds[SND_COUNT];

struct lua_State;
struct ENVIRONMENT
{
	lua_State *L;
};
extern ENVIRONMENT g_env;
