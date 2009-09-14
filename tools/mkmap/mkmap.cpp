// mkmap.cpp
// (c) Insert, 2005
//
//  версия 1.1 с поддержкой воды
/////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _USE_INLINING			// Use _tchar function inlining

#define HIMETRIC_INCH 2540
#define MAX_OBJNAME	64

#define SAFE_RELEASE(p) if(p) {p->Release(); p = NULL;}

//----------------------------------------------------

#include <stdio.h>
#include <tchar.h>
#include <windows.h>

#include <ocidl.h>
#include <olectl.h>
#include <math.h>


//----------------------------------------------------

int Difference(COLORREF color1, COLORREF color2)
{
	int r1 = color1 & 0x000000ff;
	int r2 = color2 & 0x000000ff;
	int g1 = (color1 & 0x0000ff00) >> 8;
	int g2 = (color2 & 0x0000ff00) >> 8;
	int b1 = (color1 & 0x00ff0000) >> 16;
	int b2 = (color2 & 0x00ff0000) >> 16;

	return (int) sqrt( (r1-r2)*(r1-r2) + (g1-g2)*(g1-g2) + (b1-b2)*(b1-b2) );
}

//----------------------------------------------------

struct Node
{
	COLORREF color;
	_TCHAR   name[MAX_OBJNAME];
};

//----------------------------------------------------

bool ObjectFromColor(_TCHAR *name, COLORREF color)
{
	if( 0xff000000 & color ) return false;

	const count = 4;
	static const Node nodes[count] = {
		{0xffffff, _T("WALL_CONCRETE")},
		{0x0000ff, _T("WALL")},
		{0x00ff00, _T("WOOD")},
		{0xff0000, _T("WATER")},
	};
 
	int min_d = 128;	// минимальное различие между цветами
	int index = -1;
	for( int i = 0; i < count; i++ )
	{
		int d = Difference(color, nodes[i].color);
		if( d < min_d )
		{
			min_d = d;
			index = i;
		}
	}

	if( -1 != index )
	{
		_tcscpy(name, nodes[index].name);
		return true;
	}

	return false;
}

//----------------------------------------------------

IPicture* LoadPicture(_TCHAR *filename)
{
	HANDLE hFile = CreateFile(
						filename, 
						GENERIC_READ, 
						0, 
						NULL, 
						OPEN_EXISTING, 
						FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 
						NULL
					);

	if (hFile == INVALID_HANDLE_VALUE) 
		return NULL;

	DWORD   size    = GetFileSize(hFile, NULL);
	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);

	if( NULL == hGlobal )
	{
		CloseHandle(hFile);
		return NULL;
	}

	void* pData = GlobalLock(hGlobal);

	DWORD dwBytesRead = 0;
	ReadFile(hFile, pData, size, &dwBytesRead, NULL);
	CloseHandle(hFile);

	GlobalUnlock(hGlobal);

	if( dwBytesRead != size )
	{
		GlobalFree(hGlobal);
		return NULL;
	}

	IStream* pStream = NULL;
	if( FAILED(CreateStreamOnHGlobal(hGlobal, TRUE, &pStream)) )
	{
		GlobalFree(hGlobal);
		return NULL;
	}

	IPicture *pPicture = NULL;
	OleLoadPicture(pStream, size, FALSE, IID_IPicture, (LPVOID *) &pPicture);

	SAFE_RELEASE(pStream);
	return pPicture;
}

//----------------------------------------------------

void fwassert(int expression)
{
	if( expression <= 0 ) 
		throw _T("Error writing file!\n");
}

//----------------------------------------------------

int _tmain(int argc, _TCHAR* argv[])
{
	if( argc < 2 )
	{
		_tprintf(_T("converts a picture to a tank map\n"));
		_tprintf(_T("supported formats: bmp, gif, jpg, wmf, ico\n"));
		_tprintf(_T("using: mkmap bmpfile.bmp [mapfile]\n"));
		return 0;
	}

	_tprintf(_T("input file: \n  %s\n"), argv[1]);


	IPicture *pPicture = LoadPicture(argv[1]);

	if( NULL == pPicture )
	{
		_tprintf(_T("Loading FAILED!\n"));
		return -1;
	}


	HDC hdc = GetDC(NULL);


	long height, width;
	pPicture->get_Height(&height);
	pPicture->get_Width(&width);

    long height_ = MulDiv(height, GetDeviceCaps(hdc, LOGPIXELSY), HIMETRIC_INCH);
    long width_  = MulDiv(width,  GetDeviceCaps(hdc, LOGPIXELSX), HIMETRIC_INCH);


	_tprintf(_T("width  %d px\n"), width_);
	_tprintf(_T("height %d px\n"), height_);


	HDC hdc_tmp = CreateCompatibleDC(hdc);
	HBITMAP bmp = CreateCompatibleBitmap(hdc, width_, height_);

	ReleaseDC(NULL, hdc);


	HBITMAP old = (HBITMAP) SelectObject(hdc_tmp, bmp);

	pPicture->Render(hdc_tmp, 0, 0, width_, height_, 0, height, width, -height, NULL);
    

	_TCHAR outfn[MAX_PATH];
	if( argc > 2 )
		_tcscpy(outfn, argv[2]);
	else
		_stprintf(outfn, _T("%s.tankmap"), argv[1]);

    FILE *file = _tfopen(outfn, _T("wt"));
	if( NULL != file )
	{
		_tprintf(_T("writing map to '%s'\n"), outfn);
		try 
		{
			fwassert(_ftprintf(file, _T("<tank map>\n")));
			fwassert(_ftprintf(file, _T("width %d\n"), width_));
			fwassert(_ftprintf(file, _T("height %d\n"), height_));

			_TCHAR name[MAX_OBJNAME];

			int objcount = 0;
			for( int y = 0; y < height_; y++ )
			for( int x = 0; x < width_; x++ )
			{
				if( ObjectFromColor(name, GetPixel(hdc_tmp, x, y)) )
				{
					fwassert(_ftprintf(file, _T("%s %d %d\n"), name, 16 + x * 32, 16 + y * 32));
					objcount++;
				}
			}

			_tprintf(_T("%d objects written\n"), objcount);
			_tprintf(_T("done.\n"), objcount);
		}
		catch(const _TCHAR *msg)
		{
			fclose(file);
			_tprintf(msg);
		}
	}
	else
	{
		_tprintf(_T("can't open file '%s' for writing\n"), outfn);
	}


	SelectObject(hdc_tmp, old);
	DeleteObject(bmp);
	DeleteDC(hdc_tmp);

	SAFE_RELEASE(pPicture);

	return 0;
}

