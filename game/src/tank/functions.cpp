// functions.cpp

#include "globals.h"
#include "Level.h"
#include "core/MyMath.h"
#include "fs/MapFile.h"

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

//--------------------------------------------------


uint32_t CalcCRC32(const void *data, size_t size)
{
	const uint32_t CRC_POLY = 0xEDB88320;
	const uint32_t CRC_MASK = 0xD202EF8D;

	static bool init = false;
	static uint32_t table[256];

	if( !init )
	{
		uint32_t i, j, r;
		for( i = 0; i < 256; i++ )
		{
			for( r = i, j = 8; j; j-- )
				r = (r & 1) ? ((r >> 1) ^ CRC_POLY) : (r >> 1);
			table[i] = r;
		}
		init = true;
	}

	uint32_t crc = 0;
	for( size_t i = 0; i < size; ++i )
	{
		unsigned char c = ((const unsigned char *) data)[i];
		crc = table[(crc & 0xFF) ^ c] ^ crc >> 8;
		crc ^= CRC_MASK;
	}

    return crc;
}

///////////////////////////////////////////////////////////////////////////////
// end of file
