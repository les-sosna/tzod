// functions.cpp

#include "stdafx.h"

#include "core/MyMath.h"
#include "fs/MapFile.h"

#include "ui/Interface.h"

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

//--------------------------------------------------

bool PtInFRect(const FRECT &rect, const vec2d &pt)
{
	return rect.left <= pt.x && pt.x < rect.right &&
		rect.top <= pt.y && pt.y < rect.bottom;
}

void RectToFRect(LPFRECT lpfrt, LPRECT lprt)
{
	lpfrt->left   = (float) lprt->left;
	lpfrt->top    = (float) lprt->top;
	lpfrt->right  = (float) lprt->right;
	lpfrt->bottom = (float) lprt->bottom;
}

void FRectToRect(LPRECT lprt, LPFRECT lpfrt)
{
	lprt->left   = (LONG) lpfrt->left;
	lprt->top    = (LONG) lpfrt->top;
	lprt->right  = (LONG) lpfrt->right;
	lprt->bottom = (LONG) lpfrt->bottom;
}

void OffsetFRect(LPFRECT lpfrt, float x, float y)
{
	lpfrt->left   += x;
	lpfrt->top    += y;
	lpfrt->right  += x;
	lpfrt->bottom += y;
}

void OffsetFRect(LPFRECT lpfrt, const vec2d &x)
{
	lpfrt->left   += x.x;
	lpfrt->top    += x.y;
	lpfrt->right  += x.x;
	lpfrt->bottom += x.y;
}

float frand(float max)
{
	return (float) rand() / RAND_MAX * max;
}

vec2d vrand(float len)
{
	return vec2d(frand(PI2)) * len;
}

DWORD CalcCRC32(const void *data, size_t size)
{
	const DWORD CRC_POLY = 0xEDB88320;
	const DWORD CRC_MASK = 0xD202EF8D;

	static bool init = false;
	static DWORD table[256];

	if( !init )
	{
		DWORD i, j, r;
		for( i = 0; i < 256; i++ )
		{
			for( r = i, j = 8; j; j-- )
				r = (r & 1) ? ((r >> 1) ^ CRC_POLY) : (r >> 1);
			table[i] = r;
		}
		init = true;
	}

	DWORD crc = 0;
	for( size_t i = 0; i < size; ++i )
	{
		unsigned char c = ((const unsigned char *) data)[i];
		crc = table[(crc & 0xFF) ^ c] ^ crc >> 8;
		crc ^= CRC_MASK;
	}

    return crc;
}

string_t StrFromErr(DWORD dwMessageId)
{
	LPVOID lpMsgBuf = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, dwMessageId, 0, (LPTSTR) &lpMsgBuf, 0, NULL);
	string_t result((LPCTSTR) lpMsgBuf);
	LocalFree(lpMsgBuf);
	return result;
}


///////////////////////////////////////////////////////////////////////////////
// end of file
