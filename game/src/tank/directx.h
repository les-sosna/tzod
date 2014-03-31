// directx.h

#pragma once

//----------------------------------------------------------

#ifndef NOSOUND
HRESULT InitDirectSound(HWND hWnd, bool init);
void    FreeDirectSound();
#endif

///////////////////////////////////////////////////////////////////////////////
// end of file
