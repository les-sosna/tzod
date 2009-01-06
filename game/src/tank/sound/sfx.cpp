// sfx.cpp

#include "stdafx.h"
#include "sfx.h"

#include "core/debug.h"
#include "core/Console.h"
#include "core/Application.h"

#include "macros.h"



// release retrieved data with free()
int ogg_load_vorbis(const char *filename, WAVEFORMATEX *pwf, void** ppData, int *pSize)
{
	*pSize  = 0;
	*ppData = NULL;


	FILE *file = fopen(filename, "rb");
	if( NULL == file )
	{
		// invalid file
		return -1;
	}


	OggVorbis_File  vf;
	if( ov_open(file, &vf, NULL, 0) < 0 )
	{
		// not an ogg
		fclose(file);
		return -1;
	}

	vorbis_info *pinfo = ov_info(&vf, -1);
	if( NULL == pinfo )
	{
		ov_clear(&vf);
		return -1;
	}


	pwf->wFormatTag       = WAVE_FORMAT_PCM;
	pwf->nChannels        = pinfo->channels;
	pwf->nSamplesPerSec   = pinfo->rate;
	pwf->nAvgBytesPerSec  = pinfo->rate * pinfo->channels * 2;
	pwf->nBlockAlign      = pinfo->channels * 2;
	pwf->wBitsPerSample   = 16;
	pwf->cbSize           = 0;


	*pSize  = (int) ov_pcm_total(&vf, -1) * pwf->nBlockAlign;
	*ppData = malloc(*pSize);


	char   eof = 0;
	int    current_section;
	long   TotalRet = 0, ret;

	while( TotalRet < *pSize )
	{
		ret = ov_read(&vf, (char *) *ppData + TotalRet, 
			*pSize - TotalRet, 0, 2, 1, &current_section);

		if( ret == 0 )
		{
			break; // eof
		}

		if( ret < 0 )
		{
			// error in stream
			break;
		}
		else
		{
			TotalRet += ret;
		}
	}

	ov_clear(&vf);


	if( TotalRet < 0 )
	{
		free(*ppData);
		*ppData = NULL;
		*pSize  = 0;
		
		return -1;
	}

	*pSize = TotalRet;
	return 0;
}



void LoadOggVorbis(bool init, enumSoundTemplate sound, const char *filename)
{
	if( init )
	{
		WAVEFORMATEX wfe = {0};
		void *pData = NULL;
		int size    = 0;

		if( 0 != ogg_load_vorbis(filename, &wfe, &pData, &size) )
		{
			TRACE("ERROR: couldn't load sound file '%s'\n", filename);
			//-------------------------------------------------------
			LoadSoundException e;
			e.filename = filename;
			e.hr       = E_FAIL;
			throw e;
		}

		HRESULT hr = g_soundManager->CreateFromMemory( &g_pSounds[sound],
			(BYTE *) pData, size, &wfe,
			DSBCAPS_CTRLPAN|DSBCAPS_CTRLVOLUME|DSBCAPS_CTRLFREQUENCY, GUID_NULL);

		if( FAILED(hr) )
		{
			TRACE("ERROR: couldn't create the sound buffer\n");
			//-------------------------------------------------------
			LoadSoundException e;
			e.filename = filename;
			e.hr       = hr;
			throw e;
		}
	}
	else
	{
		SAFE_DELETE(g_pSounds[sound]);
	}
}

// end of file
