// directx.h

#pragma once

//----------------------------------------------------------

#if !defined NOSOUND
HRESULT InitDirectSound(HWND hWnd, bool init);
void    FreeDirectSound();
#endif

///////////////////////////////////////////////////////////////////////////////
// end of file
