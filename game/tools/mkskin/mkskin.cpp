/* mkskin.cpp
 * make your own skin for 5 seconds!
 *  mailto:ins-games@narod.ru
 *  http://ins-games.narod.ru
 *
 *  (c) Insert 2005
 */

//----------------------------------------------------

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _USE_INLINING			// Use _tchar function inlining


#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <math.h>
#include <crtdbg.h>

//----------------------------------------------------

#define SAFE_DELETE(p) if(p) { delete p; p = NULL; }

#define FRAME_WIDTH    48	// px
#define FRAME_HEIGHT   48	// px

#define X_FRAMES	 8
#define Y_FRAMES     8

#define FRAME_COUNT (X_FRAMES * Y_FRAMES)

//----------------------------------------------------

inline RGBTRIPLE& pixel(RGBTRIPLE *pBits, int width, int x, int y)
{
	return pBits[x + y * width];
}

inline const RGBTRIPLE& pixel(const RGBTRIPLE *pBits, int width, int height, int x, int y)
{
	if( x < 0 || x >= width || y < 0 || y >= height ) 
		return pBits[0];
	return pBits[x + y * width];
}

inline RGBQUAD interpolate(RGBQUAD c[4], float x, float y)
{
	_ASSERT(x >= 0.0f && y >= 0.0f);
	_ASSERT(x <= 1.0f && y <= 1.0f);
	RGBQUAD result;
	for( int i = 0; i < 4; i++ )
	{
		float z00( *((unsigned char *) &c[0] + i) );
		float z01( *((unsigned char *) &c[1] + i) );
		float z10( *((unsigned char *) &c[2] + i) );
		float z11( *((unsigned char *) &c[3] + i) );
		float z = x*y * (z00+z11-z01-z10) + x * (z01-z00) + y * (z10-z00) + z00;
		*((unsigned char *) &result + i) = unsigned char (z);
	}
	return result;
}

inline RGBTRIPLE get_at(const RGBTRIPLE *pBits, LPBITMAPINFOHEADER pbmih, float x, float y)
{
	int ix = (int) floorf(x);
	int iy = (int) floorf(y);
	RGBQUAD c[4];
	for( int i = 0; i < 2; i++ )
	for( int j = 0; j < 2; j++ )
	{
		*((RGBTRIPLE *) &c[i+j*2]) = pixel(pBits, pbmih->biWidth, pbmih->biHeight, ix + i, iy + j);
		c[i+j*2].rgbReserved = (0 == memcmp(((RGBTRIPLE *) &c[i+j*2]), pBits, sizeof(RGBTRIPLE))) ? 0 : 255;
	}
	RGBQUAD result = interpolate(c, x - floorf(x), y - floorf(y));
	return (result.rgbReserved < 127) ? pBits[0] : *((RGBTRIPLE *) &result);
}

void MakeSkin(RGBTRIPLE *pOutputBits, const RGBTRIPLE *pInputBits, const LPBITMAPINFOHEADER pbmih)
{
	const float a = (float) (pbmih->biWidth) / (float) (FRAME_WIDTH);
	const float b = (float) (pbmih->biHeight) / (float) (FRAME_HEIGHT);
	_tprintf("processing");
	for( int y_frame = 0; y_frame < Y_FRAMES; y_frame++ )
	{
		_tprintf("..");
		for( int x_frame = 0; x_frame < X_FRAMES; x_frame++ )
		{
			float angle = (float) ((Y_FRAMES - y_frame - 1) * X_FRAMES + x_frame) / (float) FRAME_COUNT * 6.2831853f;
			float s = sinf(angle), c = cosf(angle);
			for( int y = 0; y < FRAME_HEIGHT; y++ )
            for( int x = 0; x < FRAME_WIDTH; x++ )
			{
				float x0 = (x*a - (float) pbmih->biWidth * 0.5f) + 0.5f;
				float y0 = (y*b - (float) pbmih->biHeight * 0.5f)  + 0.5f;
				pixel( pOutputBits, 
					FRAME_WIDTH * X_FRAMES, 
					x + FRAME_WIDTH * x_frame, 
					y + FRAME_HEIGHT * y_frame
				) = get_at(	pInputBits, pbmih, 
					x0 * c - y0 * s + (float) pbmih->biWidth * 0.5f - 0.5f, 
					y0 * c + x0 * s + (float) pbmih->biHeight * 0.5f - 0.5f
				);
			}
		}
	}
	_tprintf(" done!\n");
}

bool ReadBitmap(const _TCHAR *pFileName, LPBITMAPFILEHEADER pbmfh, LPBITMAPINFOHEADER pbmih, RGBTRIPLE **ppBits)
{
	_ASSERT(NULL == *ppBits);
	bool result = true;
	FILE *file = _tfopen(pFileName, _T("rb"));
	try
	{
		if( NULL == file )
			throw _T("can't open file for reading\n");
		if( 1 != fread(pbmfh, sizeof(BITMAPFILEHEADER), 1, file) ||
			1 != fread(pbmih, sizeof(BITMAPINFOHEADER), 1, file) ) 
			throw _T("invalid file\n");
		if( 24 != pbmih->biBitCount ) 
			throw _T("only 24 bit bitmaps supported\n");
		long pixel_count = pbmih->biWidth * pbmih->biHeight;
		*ppBits = new RGBTRIPLE[pixel_count];
		fseek(file, pbmfh->bfOffBits, SEEK_SET);
		if( pixel_count != fread(*ppBits, sizeof(RGBTRIPLE), pixel_count, file) )
			throw _T("unexpected end of file\n");
	}
	catch(_TCHAR *msg)
	{
		SAFE_DELETE(*ppBits);
		result = false;
		_tprintf(msg);
	}
	if( file ) fclose(file);
	return result;
}

bool WriteBitmap(const _TCHAR* pFileName, const LPBITMAPFILEHEADER pbmfh, const LPBITMAPINFOHEADER pbmih, const RGBTRIPLE *pBits)
{
	bool result = true;
	FILE *file = _tfopen(pFileName, "wb");
	try
	{
		if( NULL == file )
			throw _T("can't open file for writing\n");
		if( 1 != fwrite(pbmfh, sizeof(BITMAPFILEHEADER), 1, file) ||
			1 != fwrite(pbmih, sizeof(BITMAPINFOHEADER), 1, file) ) 
			throw _T("error writing file\n");
		int skip = pbmfh->bfOffBits - sizeof(BITMAPFILEHEADER) - sizeof(BITMAPINFOHEADER);
		if( skip > 0 )
		{
			char empty = 0;
			if( skip != fwrite(&empty, 1, skip, file) )
				throw _T("error writing file\n");
		}
		fseek(file, pbmfh->bfOffBits, SEEK_SET);
		long pixel_count = pbmih->biWidth * pbmih->biHeight;
		if( pixel_count != fwrite(pBits, sizeof(RGBTRIPLE), pixel_count, file) )
			throw _T("error writing file\n");
	}
	catch(_TCHAR *msg)
	{
		result = false;
		_tprintf(msg);
	}
	if( file ) fclose(file);
	return result;
}

int _tmain(int argc, _TCHAR* argv[])
{
	if( argc < 2 )
	{
		_tprintf(_T("converts a picture to a tank skin\n"));
		_tprintf(_T("only 24-bit windows bitmaps supports\n"));
		_tprintf(_T("using: mkskin input_file.bmp [output_file.bmp]\n"));
		return 0;
	}

	_tprintf(_T("input file: \n  %s\n"), argv[1]);

	BITMAPFILEHEADER bmfh	= {0};
	BITMAPINFOHEADER bmih	= {0};

	RGBTRIPLE *pInputBits = NULL;
	if( !ReadBitmap(argv[1], &bmfh, &bmih, &pInputBits) )
		return -1;

	RGBTRIPLE *pOutputBits = new RGBTRIPLE[FRAME_WIDTH * FRAME_HEIGHT * FRAME_COUNT];
    MakeSkin(pOutputBits, pInputBits, &bmih);

	bmih.biSizeImage = FRAME_WIDTH * FRAME_HEIGHT * FRAME_COUNT * sizeof(RGBTRIPLE);
	bmfh.bfSize      = bmih.biSizeImage + bmfh.bfOffBits;
	bmih.biWidth     = FRAME_WIDTH * X_FRAMES;
	bmih.biHeight    = FRAME_HEIGHT * Y_FRAMES;

	_TCHAR outfn[MAX_PATH];
	if( argc > 2 ) _tcscpy(outfn, argv[2]);
	else _stprintf(outfn, _T("%s-skin.bmp"), argv[1]);

	_tprintf(_T("output file: \n  %s\n"), outfn);
	if( WriteBitmap(outfn, &bmfh, &bmih, pOutputBits) )
		_tprintf(_T("OK.\n"));
	else
		Sleep(1000);

	delete[] pOutputBits;
	delete[] pInputBits;
	return 0;
}
