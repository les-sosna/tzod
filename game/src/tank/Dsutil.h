//-----------------------------------------------------------------------------
// File: DSUtil.h
//
// Desc:
//
// Copyright (c) 1999-2000 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef DSUTIL_H
#define DSUTIL_H

#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <dsound.h>




//-----------------------------------------------------------------------------
// Classes used by this header
//-----------------------------------------------------------------------------
class CSoundManager;
class CSound;
class CWaveFile;




//-----------------------------------------------------------------------------
// Typing macros
//-----------------------------------------------------------------------------
#define WAVEFILE_READ   1


//-----------------------------------------------------------------------------
// Name: class CSoundManager
// Desc:
//-----------------------------------------------------------------------------
class CSoundManager
{
protected:
    LPDIRECTSOUND8      _dS;
    LPDIRECTSOUNDBUFFER _dSBPrimary;

public:
    CSoundManager();
    ~CSoundManager();

    HRESULT Initialize( HWND hWnd, DWORD dwCoopLevel, DWORD dwPrimaryChannels, DWORD dwPrimaryFreq, DWORD dwPrimaryBitRate );
    inline  LPDIRECTSOUND8 GetDirectSound() { return _dS; }
    HRESULT SetPrimaryBufferFormat( DWORD dwPrimaryChannels, DWORD dwPrimaryFreq, DWORD dwPrimaryBitRate );
    HRESULT Get3DListenerInterface( LPDIRECTSOUND3DLISTENER* ppDSListener );

	HRESULT SetVolume(LONG volume)
	{
		return _dSBPrimary->SetVolume(volume);
	};

	HRESULT GetVolume(LPLONG lpVolume)
	{
		return _dSBPrimary->GetVolume(lpVolume);
	};

    HRESULT CreateFromMemory( CSound** ppSound, BYTE* pbData, ULONG ulDataSize, LPWAVEFORMATEX pwfx, DWORD dwCreationFlags = 0, GUID guid3DAlgorithm = GUID_NULL, DWORD dwNumBuffers = 1 );
};




//-----------------------------------------------------------------------------
// Name: class CSound
// Desc: Encapsulates functionality of a DirectSound buffer.
//-----------------------------------------------------------------------------
class CSound
{
protected:
    LPDIRECTSOUNDBUFFER* _apDSBuffer;
    DWORD                _dwDSBufferSize;
    CWaveFile*           _waveFile;
    DWORD                _dwNumBuffers;

    HRESULT RestoreBuffer( LPDIRECTSOUNDBUFFER pDSB, BOOL* pbWasRestored );

public:
    CSound( LPDIRECTSOUNDBUFFER* apDSBuffer, DWORD dwDSBufferSize, DWORD dwNumBuffers, CWaveFile* pWaveFile );
    virtual ~CSound();

    HRESULT Get3DBufferInterface( DWORD dwIndex, LPDIRECTSOUND3DBUFFER* ppDS3DBuffer );
    HRESULT FillBufferWithSound( LPDIRECTSOUNDBUFFER pDSB, BOOL bRepeatWavIfBufferLarger );
    LPDIRECTSOUNDBUFFER GetFreeBuffer();
    LPDIRECTSOUNDBUFFER GetBuffer( DWORD dwIndex );

    HRESULT Play( DWORD dwPriority, DWORD dwFlags );
    HRESULT Stop();
    HRESULT Reset();
};



//-----------------------------------------------------------------------------
// Name: class CWaveFile
// Desc: Encapsulates reading or writing sound data to or from a wave file
//-----------------------------------------------------------------------------
class CWaveFile
{
public:
    WAVEFORMATEX* _pwfx;        // Pointer to WAVEFORMATEX structure
    BYTE*         _pbData;
    BYTE*         _pbDataCur;
    ULONG         _ulDataSize;

public:
    CWaveFile();

    HRESULT OpenFromMemory( BYTE* pbData, ULONG ulDataSize, WAVEFORMATEX* pwfx );

    HRESULT Read( BYTE* pBuffer, DWORD dwSizeToRead, DWORD* pdwSizeRead );

    HRESULT ResetFile();
};


#endif // DSUTIL_H
