// functions.cpp

#include "stdafx.h"

#include "core/MyMath.h"
#include "fs/MapFile.h"

#include "Level.h"

#include "ui/Interface.h"


//--------------------------------------------------

void CalcOutstrip(const vec2d &fp, // fire point
				  float vp,        // speed of the projectile
				  const vec2d &tx, // target position
				  const vec2d &tv, // target velocity
				  vec2d &fake)     // out: fake target position
{
	float vt = tv.Length();

	if( vt >= vp || vt < 1e-7 )
	{
		fake = tx;
	}
	else
	{
		float cg = tv.x / vt;
		float sg = tv.y / vt;

		float x   = (tx.x - fp.x) * cg + (tx.y - fp.y) * sg;
		float y   = (tx.y - fp.y) * cg - (tx.x - fp.x) * sg;
		float tmp = vp*vp - vt*vt;

		float fx = x + vt * (x*vt + sqrtf(x*x * vp*vp + y*y * tmp)) / tmp;

		fake.x = __max(0, __min(g_level->_sx, fp.x + fx*cg - y*sg));
		fake.y = __max(0, __min(g_level->_sy, fp.y + fx*sg + y*cg));
	}
}

//--------------------------------------------------

// проверка на пересечение правильных пр€моугольников
BOOL IsIntersect(LPFRECT lprtRect1, LPFRECT lprtRect2)
{
	float l, r, t, b;

	l = __max(lprtRect1->left,   lprtRect2->left   );
	r = __min(lprtRect1->right,  lprtRect2->right  );

	if( !(l < r) ) return FALSE;

	t = __max(lprtRect1->top,    lprtRect2->top    );
	b = __min(lprtRect1->bottom, lprtRect2->bottom );

	if( t < b ) return TRUE;

	return FALSE;
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

// генераци€ случайного числа от 0 до max
float frand(float max)
{
	return (float) rand() / RAND_MAX * max;
}

// генераци€ случайно направленого вектора длиной len
vec2d vrand(float len)
{
	return vec2d(frand(PI2)) * len;
}

int net_rand()
{
	g_level->_seed = (69069 * g_level->_seed + 1);
	return g_level->_seed & RAND_MAX;
}

float net_frand(float max)
{
	return (float) net_rand() / RAND_MAX * max;
}

vec2d net_vrand(float len)
{
	return vec2d(net_frand(PI2)) * len;
}


// если каталог не существует, то пользователь
// получит запрос на создание этого каталога
//      return TRUE - удачно
BOOL SafeSetCurDir(LPCTSTR lpstrName, HWND hDlg)
{
	if( !SetCurrentDirectory(lpstrName) )
	{
		char s[MAX_PATH];
		wsprintf(s, "Ќе найден каталог '%s'. \n\tCоздать его?", lpstrName);

		if( IDYES == MessageBoxT(hDlg, s, MB_YESNO|MB_ICONSTOP) )
		{
			if( !CreateDirectory(lpstrName, NULL) )
			{
				wsprintf(s, "Ќе удаетс€ создать каталог %s. ¬озможно диск переполнен или защищен от записи", lpstrName);
				MessageBoxT(hDlg, s, MB_OK|MB_ICONSTOP);
				return FALSE;
			}

			SetCurrentDirectory(lpstrName);
		}
		else
		{
			return FALSE;
		}
	}

	return TRUE;
}

/*
// проверка файла на корректность
BOOL CheckFile_ie(LPCTSTR fileName)
{
	MapFile file;
	if( file.Open(fileName, false) )
		return TRUE;

	return FALSE;
}

BOOL CheckFile_ls(LPCTSTR fileName)
{
	DWORD dwBytesReaded = 0;

	HANDLE file = CreateFile(
						fileName,
						GENERIC_READ,
						0,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
						NULL);

	if( file == INVALID_HANDLE_VALUE )
	{
		return FALSE;
	}

	//провер€ем версию файла
	DWORD dwVersion;

	ReadFile(file, &dwVersion, sizeof(DWORD), &dwBytesReaded, NULL);
	CloseHandle(file);

	return (VERSION == dwVersion);
}
*/

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
		return -1;

	DWORD crc = 0;
	while( true )
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
