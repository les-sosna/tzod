// directx.h

#pragma once

//----------------------------------------------------------

VOID    LoadSurfaces();

#if !defined NOSOUND
HRESULT InitDirectSound(HWND hWnd, bool init);
void    FreeDirectSound();
#endif


HRESULT InitDirectInput(HWND hWnd);
void    FreeDirectInput();
HRESULT ReadImmediateData();
void    GetKeyName(int nKey, char *pBuf);

/////////////////////////////////////////
// инициализация всех интерфейсов DirectX
HRESULT InitAll( HWND hWnd );


///////////////////////////////////////////////////////////////////////////////
// end of file
