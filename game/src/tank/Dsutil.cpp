//-----------------------------------------------------------------------------
// File: DSUtil.cpp
//
// Desc: DirectSound framework classes for reading and writing wav files and
//       playing them in DirectSound buffers. Feel free to use this class
//       as a starting point for adding extra functionality.
//
// Copyright (c) 1999-2000 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"

#include <windows.h>
#include <mmsystem.h>
#include <dxerr.h>
#include <dsound.h>

#include "DSUtil.h"
#include "macros.h"

#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }



//-----------------------------------------------------------------------------
// Name: CSoundManager::CSoundManager()
// Desc: Constructs the class
//-----------------------------------------------------------------------------
CSoundManager::CSoundManager()
{
    _dS         = NULL;
	_dSBPrimary = NULL;
}




//-----------------------------------------------------------------------------
// Name: CSoundManager::~CSoundManager()
// Desc: Destroys the class
//-----------------------------------------------------------------------------
CSoundManager::~CSoundManager()
{
	SAFE_RELEASE( _dSBPrimary );
    SAFE_RELEASE( _dS );
}




//-----------------------------------------------------------------------------
// Name: CSoundManager::Initialize()
// Desc: Initializes the IDirectSound object and also sets the primary buffer
//       format.  This function must be called before any others.
//-----------------------------------------------------------------------------
HRESULT CSoundManager::Initialize( HWND  hWnd,
                                   DWORD dwCoopLevel,
                                   DWORD dwPrimaryChannels,
                                   DWORD dwPrimaryFreq,
                                   DWORD dwPrimaryBitRate )
{
    HRESULT             hr;
    LPDIRECTSOUNDBUFFER pDSBPrimary = NULL;

    SAFE_RELEASE( _dS );

    // Create IDirectSound using the primary sound device
    if( FAILED( hr = DirectSoundCreate8( NULL, &_dS, NULL ) ) )
        return DXTRACE_ERR( TEXT("DirectSoundCreate8"), hr );

    // Set DirectSound coop level
    if( FAILED( hr = _dS->SetCooperativeLevel( hWnd, dwCoopLevel ) ) )
        return DXTRACE_ERR( TEXT("SetCooperativeLevel"), hr );

    // Set primary buffer format
	SetPrimaryBufferFormat( dwPrimaryChannels, dwPrimaryFreq, dwPrimaryBitRate );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: CSoundManager::SetPrimaryBufferFormat()
// Desc: Set primary buffer to a specified format
//       For example, to set the primary buffer format to 22kHz stereo, 16-bit
//       then:   dwPrimaryChannels = 2
//               dwPrimaryFreq     = 22050,
//               dwPrimaryBitRate  = 16
//-----------------------------------------------------------------------------
HRESULT CSoundManager::SetPrimaryBufferFormat( DWORD dwPrimaryChannels,
                                               DWORD dwPrimaryFreq,
                                               DWORD dwPrimaryBitRate )
{
    HRESULT             hr;
//    LPDIRECTSOUNDBUFFER pDSBPrimary = NULL;

    if( _dS == NULL )
        return CO_E_NOTINITIALIZED;

	SAFE_RELEASE( _dSBPrimary );

    // Get the primary buffer
    DSBUFFERDESC dsbd;
    ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
    dsbd.dwSize        = sizeof(DSBUFFERDESC);
    dsbd.dwFlags       = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
    dsbd.dwBufferBytes = 0;
    dsbd.lpwfxFormat   = NULL;

    if( FAILED( hr = _dS->CreateSoundBuffer( &dsbd, &_dSBPrimary, NULL ) ) )
        return DXTRACE_ERR( TEXT("CreateSoundBuffer"), hr );

    WAVEFORMATEX wfx;
    ZeroMemory( &wfx, sizeof(WAVEFORMATEX) );
    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.nChannels       = (WORD) dwPrimaryChannels;
    wfx.nSamplesPerSec  = dwPrimaryFreq;
    wfx.wBitsPerSample  = (WORD) dwPrimaryBitRate;
    wfx.nBlockAlign     = wfx.wBitsPerSample / 8 * wfx.nChannels;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    if( FAILED( hr = _dSBPrimary->SetFormat(&wfx) ) )
        return DXTRACE_ERR( TEXT("SetFormat"), hr );

   // SAFE_RELEASE( pDSBPrimary );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: CSoundManager::Get3DListenerInterface()
// Desc: Returns the 3D listener interface associated with primary buffer.
//-----------------------------------------------------------------------------
HRESULT CSoundManager::Get3DListenerInterface( LPDIRECTSOUND3DLISTENER* ppDSListener )
{
    HRESULT             hr;
    DSBUFFERDESC        dsbdesc;
    LPDIRECTSOUNDBUFFER pDSBPrimary = NULL;

    if( ppDSListener == NULL )
        return E_INVALIDARG;
    if( _dS == NULL )
        return CO_E_NOTINITIALIZED;

    *ppDSListener = NULL;

    // Obtain primary buffer, asking it for 3D control
    ZeroMemory( &dsbdesc, sizeof(DSBUFFERDESC) );
    dsbdesc.dwSize = sizeof(DSBUFFERDESC);
    dsbdesc.dwFlags = DSBCAPS_CTRL3D | DSBCAPS_PRIMARYBUFFER;
    if( FAILED( hr = _dS->CreateSoundBuffer( &dsbdesc, &pDSBPrimary, NULL ) ) )
        return DXTRACE_ERR( TEXT("CreateSoundBuffer"), hr );

    if( FAILED( hr = pDSBPrimary->QueryInterface( IID_IDirectSound3DListener,
                                                  (VOID**)ppDSListener ) ) )
    {
        SAFE_RELEASE( pDSBPrimary );
        return DXTRACE_ERR( TEXT("QueryInterface"), hr );
    }

    // Release the primary buffer, since it is not need anymore
    SAFE_RELEASE( pDSBPrimary );

    return S_OK;
}



//-----------------------------------------------------------------------------
// Name: CSoundManager::CreateFromMemory()
// Desc:
//-----------------------------------------------------------------------------
HRESULT CSoundManager::CreateFromMemory( CSound** ppSound,
                                        BYTE* pbData,
                                        ULONG  ulDataSize,
                                        LPWAVEFORMATEX pwfx,
                                        DWORD dwCreationFlags,
                                        GUID guid3DAlgorithm,
                                        DWORD dwNumBuffers )
{
    HRESULT hr;
    DWORD   i;
    LPDIRECTSOUNDBUFFER* apDSBuffer     = NULL;
    DWORD                dwDSBufferSize = NULL;
    CWaveFile*           pWaveFile      = NULL;

    if( _dS == NULL )
        return CO_E_NOTINITIALIZED;
    if( pbData == NULL || ppSound == NULL || dwNumBuffers < 1 )
        return E_INVALIDARG;

    apDSBuffer = new LPDIRECTSOUNDBUFFER[dwNumBuffers];
    pWaveFile = new CWaveFile();
    pWaveFile->OpenFromMemory( pbData,ulDataSize, pwfx );


    // Make the DirectSound buffer the same size as the wav file
    dwDSBufferSize = ulDataSize;

    // Create the direct sound buffer, and only request the flags needed
    // since each requires some overhead and limits if the buffer can
    // be hardware accelerated
    DSBUFFERDESC dsbd;
    ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
    dsbd.dwSize          = sizeof(DSBUFFERDESC);
    dsbd.dwFlags         = dwCreationFlags;
    dsbd.dwBufferBytes   = dwDSBufferSize;
    dsbd.guid3DAlgorithm = guid3DAlgorithm;
    dsbd.lpwfxFormat     = pwfx;

    if( FAILED( hr = _dS->CreateSoundBuffer( &dsbd, &apDSBuffer[0], NULL ) ) )
    {
        DXTRACE_ERR( TEXT("CreateSoundBuffer"), hr );
        goto LFail;
    }

    for( i=1; i<dwNumBuffers; i++ )
    {
        if( FAILED( hr = _dS->DuplicateSoundBuffer( apDSBuffer[0], &apDSBuffer[i] ) ) )
        {
            DXTRACE_ERR( TEXT("DuplicateSoundBuffer"), hr );
            goto LFail;
        }
    }

    // Create the sound
    *ppSound = new CSound( apDSBuffer, dwDSBufferSize, dwNumBuffers, pWaveFile );

    SAFE_DELETE( apDSBuffer );
    return S_OK;

LFail:
    // Cleanup

    SAFE_DELETE( apDSBuffer );
    return hr;
}




//-----------------------------------------------------------------------------
// Name: CSound::CSound()
// Desc: Constructs the class
//-----------------------------------------------------------------------------
CSound::CSound( LPDIRECTSOUNDBUFFER* apDSBuffer, DWORD dwDSBufferSize,
                DWORD dwNumBuffers, CWaveFile* pWaveFile )
{
    DWORD i;

    _apDSBuffer = new LPDIRECTSOUNDBUFFER[dwNumBuffers];
    for( i=0; i<dwNumBuffers; i++ )
        _apDSBuffer[i] = apDSBuffer[i];

    _dwDSBufferSize = dwDSBufferSize;
    _dwNumBuffers   = dwNumBuffers;
    _waveFile      = pWaveFile;

    FillBufferWithSound( _apDSBuffer[0], FALSE );

    // Make DirectSound do pre-processing on sound effects
    for( i=0; i<dwNumBuffers; i++ )
        _apDSBuffer[i]->SetCurrentPosition(0);
}




//-----------------------------------------------------------------------------
// Name: CSound::~CSound()
// Desc: Destroys the class
//-----------------------------------------------------------------------------
CSound::~CSound()
{
    for( DWORD i=0; i<_dwNumBuffers; i++ )
    {
        SAFE_RELEASE( _apDSBuffer[i] );
    }

    SAFE_DELETE_ARRAY( _apDSBuffer );
    SAFE_DELETE( _waveFile );
}




//-----------------------------------------------------------------------------
// Name: CSound::FillBufferWithSound()
// Desc: Fills a DirectSound buffer with a sound file
//-----------------------------------------------------------------------------
HRESULT CSound::FillBufferWithSound( LPDIRECTSOUNDBUFFER pDSB, BOOL bRepeatWavIfBufferLarger )
{
    HRESULT hr;
    VOID*   pDSLockedBuffer      = NULL; // Pointer to locked buffer memory
    DWORD   dwDSLockedBufferSize = 0;    // Size of the locked DirectSound buffer
    DWORD   dwWavDataRead        = 0;    // Amount of data read from the wav file

    if( pDSB == NULL )
        return CO_E_NOTINITIALIZED;

    // Make sure we have focus, and we didn't just switch in from
    // an app which had a DirectSound device
    if( FAILED( hr = RestoreBuffer( pDSB, NULL ) ) )
        return DXTRACE_ERR( TEXT("RestoreBuffer"), hr );

    // Lock the buffer down
    if( FAILED( hr = pDSB->Lock( 0, _dwDSBufferSize,
                                 &pDSLockedBuffer, &dwDSLockedBufferSize,
                                 NULL, NULL, 0L ) ) )
        return DXTRACE_ERR( TEXT("Lock"), hr );

    // Reset the wave file to the beginning
    _waveFile->ResetFile();

    if( FAILED( hr = _waveFile->Read( (BYTE*) pDSLockedBuffer,
                                        dwDSLockedBufferSize,
                                        &dwWavDataRead ) ) )
        return DXTRACE_ERR( TEXT("Read"), hr );

    if( dwWavDataRead == 0 )
    {
        // Wav is blank, so just fill with silence
        FillMemory( (BYTE*) pDSLockedBuffer,
                    dwDSLockedBufferSize,
                    (BYTE)(_waveFile->_pwfx->wBitsPerSample == 8 ? 128 : 0 ) );
    }
    else if( dwWavDataRead < dwDSLockedBufferSize )
    {
        // If the wav file was smaller than the DirectSound buffer,
        // we need to fill the remainder of the buffer with data
        if( bRepeatWavIfBufferLarger )
        {
            // Reset the file and fill the buffer with wav data
            DWORD dwReadSoFar = dwWavDataRead;    // From previous call above.
            while( dwReadSoFar < dwDSLockedBufferSize )
            {
                // This will keep reading in until the buffer is full
                // for very short files
                if( FAILED( hr = _waveFile->ResetFile() ) )
                    return DXTRACE_ERR( TEXT("ResetFile"), hr );

                hr = _waveFile->Read( (BYTE*)pDSLockedBuffer + dwReadSoFar,
                                        dwDSLockedBufferSize - dwReadSoFar,
                                        &dwWavDataRead );
                if( FAILED(hr) )
                    return DXTRACE_ERR( TEXT("Read"), hr );

                dwReadSoFar += dwWavDataRead;
            }
        }
        else
        {
            // Don't repeat the wav file, just fill in silence
            FillMemory( (BYTE*) pDSLockedBuffer + dwWavDataRead,
                        dwDSLockedBufferSize - dwWavDataRead,
                        (BYTE)(_waveFile->_pwfx->wBitsPerSample == 8 ? 128 : 0 ) );
        }
    }

    // Unlock the buffer, we don't need it anymore.
    pDSB->Unlock( pDSLockedBuffer, dwDSLockedBufferSize, NULL, 0 );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: CSound::RestoreBuffer()
// Desc: Restores the lost buffer. *pbWasRestored returns TRUE if the buffer was
//       restored.  It can also NULL if the information is not needed.
//-----------------------------------------------------------------------------
HRESULT CSound::RestoreBuffer( LPDIRECTSOUNDBUFFER pDSB, BOOL* pbWasRestored )
{
    HRESULT hr;

    if( pDSB == NULL )
        return CO_E_NOTINITIALIZED;
    if( pbWasRestored )
        *pbWasRestored = FALSE;

    DWORD dwStatus;
    if( FAILED( hr = pDSB->GetStatus( &dwStatus ) ) )
        return DXTRACE_ERR( TEXT("GetStatus"), hr );

    if( dwStatus & DSBSTATUS_BUFFERLOST )
    {
        // Since the app could have just been activated, then
        // DirectSound may not be giving us control yet, so
        // the restoring the buffer may fail.
        // If it does, sleep until DirectSound gives us control.
        do
        {
            hr = pDSB->Restore();
            if( hr == DSERR_BUFFERLOST )
                Sleep( 10 );
        }
        while( hr = pDSB->Restore() );

        if( pbWasRestored != NULL )
            *pbWasRestored = TRUE;

        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}




//-----------------------------------------------------------------------------
// Name: CSound::GetFreeBuffer()
// Desc: Checks to see if a buffer is playing and returns TRUE if it is.
//-----------------------------------------------------------------------------
LPDIRECTSOUNDBUFFER CSound::GetFreeBuffer()
{
    BOOL bIsPlaying = FALSE;

    if( _apDSBuffer == NULL )
        return FALSE;

	DWORD i=0;
    for( ; i<_dwNumBuffers; i++ )
    {
        if( _apDSBuffer[i] )
        {
            DWORD dwStatus = 0;
            _apDSBuffer[i]->GetStatus( &dwStatus );
            if( ( dwStatus & DSBSTATUS_PLAYING ) == 0 )
                break;
        }
    }

    if( i != _dwNumBuffers )
        return _apDSBuffer[ i ];
    else
        return _apDSBuffer[ rand() % _dwNumBuffers ];
}




//-----------------------------------------------------------------------------
// Name: CSound::GetBuffer()
// Desc:
//-----------------------------------------------------------------------------
LPDIRECTSOUNDBUFFER CSound::GetBuffer( DWORD dwIndex )
{
    if( _apDSBuffer == NULL )
        return NULL;
    if( dwIndex >= _dwNumBuffers )
        return NULL;

    return _apDSBuffer[dwIndex];
}




//-----------------------------------------------------------------------------
// Name: CSound::Get3DBufferInterface()
// Desc:
//-----------------------------------------------------------------------------
HRESULT CSound::Get3DBufferInterface( DWORD dwIndex, LPDIRECTSOUND3DBUFFER* ppDS3DBuffer )
{
    if( _apDSBuffer == NULL )
        return CO_E_NOTINITIALIZED;
    if( dwIndex >= _dwNumBuffers )
        return E_INVALIDARG;

    *ppDS3DBuffer = NULL;

    return _apDSBuffer[dwIndex]->QueryInterface( IID_IDirectSound3DBuffer,
                                                  (VOID**)ppDS3DBuffer );
}


//-----------------------------------------------------------------------------
// Name: CSound::Play()
// Desc: Plays the sound using voice management flags.  Pass in DSBPLAY_LOOPING
//       in the dwFlags to loop the sound
//-----------------------------------------------------------------------------
HRESULT CSound::Play( DWORD dwPriority, DWORD dwFlags )
{
    HRESULT hr;
    BOOL    bRestored;

    if( _apDSBuffer == NULL )
        return CO_E_NOTINITIALIZED;

    LPDIRECTSOUNDBUFFER pDSB = GetFreeBuffer();

    if( pDSB == NULL )
        return DXTRACE_ERR( TEXT("GetFreeBuffer"), E_FAIL );

    // Restore the buffer if it was lost
    if( FAILED( hr = RestoreBuffer( pDSB, &bRestored ) ) )
        return DXTRACE_ERR( TEXT("RestoreBuffer"), hr );

    if( bRestored )
    {
        // The buffer was restored, so we need to fill it with new data
        if( FAILED( hr = FillBufferWithSound( pDSB, FALSE ) ) )
            return DXTRACE_ERR( TEXT("FillBufferWithSound"), hr );

        // Make DirectSound do pre-processing on sound effects
        Reset();
    }

    return pDSB->Play( 0, dwPriority, dwFlags );
}




//-----------------------------------------------------------------------------
// Name: CSound::Stop()
// Desc: Stops the sound from playing
//-----------------------------------------------------------------------------
HRESULT CSound::Stop()
{
    if( _apDSBuffer == NULL )
        return CO_E_NOTINITIALIZED;

    HRESULT hr = 0;

    for( DWORD i=0; i<_dwNumBuffers; i++ )
        hr |= _apDSBuffer[i]->Stop();

    return hr;
}




//-----------------------------------------------------------------------------
// Name: CSound::Reset()
// Desc: Reset all of the sound buffers
//-----------------------------------------------------------------------------
HRESULT CSound::Reset()
{
    if( _apDSBuffer == NULL )
        return CO_E_NOTINITIALIZED;

    HRESULT hr = 0;

    for( DWORD i=0; i<_dwNumBuffers; i++ )
        hr |= _apDSBuffer[i]->SetCurrentPosition( 0 );

    return hr;
}






//-----------------------------------------------------------------------------
// Name: CWaveFile::CWaveFile()
// Desc: Constructs the class.  Call Open() to open a wave file for reading.
//       Then call Read() as needed.  Calling the destructor or Close()
//       will close the file.
//-----------------------------------------------------------------------------
CWaveFile::CWaveFile()
{
    _pwfx    = NULL;
}


HRESULT CWaveFile::OpenFromMemory( BYTE* pbData, ULONG ulDataSize, WAVEFORMATEX* pwfx )
{
    _pwfx       = pwfx;
    _ulDataSize = ulDataSize;
    _pbData     = pbData;
    _pbDataCur  = _pbData;

	return S_OK;
}


HRESULT CWaveFile::ResetFile()
{
    _pbDataCur = _pbData;
    return S_OK;
}


HRESULT CWaveFile::Read( BYTE* pBuffer, DWORD dwSizeToRead, DWORD* pdwSizeRead )
{
    if( _pbDataCur == NULL )
        return CO_E_NOTINITIALIZED;
    if( pdwSizeRead != NULL )
        *pdwSizeRead = 0;

    if( (BYTE*)(_pbDataCur + dwSizeToRead) >
        (BYTE*)(_pbData + _ulDataSize) )
    {
        dwSizeToRead = _ulDataSize - (DWORD)(_pbDataCur - _pbData);
    }

    CopyMemory( pBuffer, _pbDataCur, dwSizeToRead );

    if( pdwSizeRead != NULL )
        *pdwSizeRead = dwSizeToRead;

    return S_OK;
}


