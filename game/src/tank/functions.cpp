// functions.cpp

#include "stdafx.h"

#include "core/MyMath.h"
#include "fs/MapFile.h"

#include "ui/Interface.h"

#include "Level.h"

bool PauseGame(bool pause)
{
	if( pause )
	{
		if( 0 == g_level->_pause + g_env.pause )
		{
			g_level->PauseSound(true);
		}
		g_env.pause++;
	}
	else
	{
		_ASSERT(g_env.pause > 0);
		g_env.pause--;
		if( 0 == g_level->_pause + g_env.pause )
		{
			g_level->PauseSound(false);
		}
	}
	return g_env.pause > 0;
}

//--------------------------------------------------

// проверка на пересечение правильных прямоугольников
bool IsIntersect(LPFRECT lprtRect1, LPFRECT lprtRect2)
{
	float l, r, t, b;

	l = __max(lprtRect1->left,   lprtRect2->left   );
	r = __min(lprtRect1->right,  lprtRect2->right  );

	if( !(l < r) ) return false;

	t = __max(lprtRect1->top,    lprtRect2->top    );
	b = __min(lprtRect1->bottom, lprtRect2->bottom );

	if( t < b ) return true;

	return false;
}

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

// генерация случайного числа от 0 до max
float frand(float max)
{
	return (float) rand() / RAND_MAX * max;
}

// генерация случайно направленого вектора длиной len
vec2d vrand(float len)
{
	return vec2d(frand(PI2)) * len;
}

// если каталог не существует, то пользователь
// получит запрос на создание этого каталога
//      return TRUE - удачно
BOOL SafeSetCurDir(LPCTSTR lpstrName, HWND hDlg)
{
	if( !SetCurrentDirectory(lpstrName) )
	{
		char s[MAX_PATH];
		if( !CreateDirectory(lpstrName, NULL) )
		{
			wsprintf(s, "Could not create directory '%s'", lpstrName);
			MessageBoxT(hDlg, s, MB_OK|MB_ICONSTOP);
			return FALSE;
		}
		SetCurrentDirectory(lpstrName);
	}

	return TRUE;
}

DWORD CalcCRC32(LPCTSTR fileName)
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

	FILE *file = fopen(fileName, "rb");
	if( NULL == file )
		return 0xffffffff;

	DWORD crc = 0;
	for(;;)
	{
		unsigned char data;
		if( 1 != fread(&data, 1, 1, file) )
			break;
		crc = table[(crc & 0xFF) ^ data] ^ crc >> 8;
		crc ^= CRC_MASK;
	}

	fclose(file);
    return crc;
}

///////////////////////////////////////////////////////////////////////////////
// end of file
