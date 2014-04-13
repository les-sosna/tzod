// Pause.cpp

#include "globals.h"
#include "Level.h"

bool PauseGame(bool pause)
{
	if( g_level )
	{
		bool pausedBefore = g_level->IsGamePaused();
		g_env.pause = pause ? g_env.pause + 1 : g_env.pause - 1;
		if( !g_level->IsGamePaused() ^ !pausedBefore )
		{
			g_level->PauseSound(g_level->IsGamePaused());
		}
	}
	else
	{
		g_env.pause = pause ? g_env.pause + 1 : g_env.pause - 1;
	}
	assert(g_env.pause >= 0);
	return g_env.pause > 0;
}

///////////////////////////////////////////////////////////////////////////////
// end of file
