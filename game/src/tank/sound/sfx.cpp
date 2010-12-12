// sfx.cpp

#include "stdafx.h"
#include "sfx.h"

#include "core/debug.h"

#include "fs/FileSystem.h"

#include "globals.h"
#include "macros.h"


static size_t read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	FS::Stream *s = (FS::Stream *) datasource;
	return s->Read(ptr, size, nmemb);
}

static int seek_func(void *datasource, ogg_int64_t offset, int whence)
{
	FS::Stream *s = (FS::Stream *) datasource;
	s->Seek(offset, whence);
	return 0;
}

static long tell_func(void *datasource)
{
	FS::Stream *s = (FS::Stream *) datasource;
	return s->Seek(0, SEEK_CUR);
}

static void ogg_load_vorbis(const char *filename, WAVEFORMATEX *pwf, std::vector<char> *data)
{
	SafePtr<FS::Stream> s = g_fs->Open(filename)->QueryStream();

	ov_callbacks cb;
	cb.read_func  = read_func;
	cb.seek_func  = seek_func;
	cb.close_func = NULL;
	cb.tell_func  = tell_func;

	OggVorbis_File vf;
	if( int result = ov_open_callbacks(s, &vf, NULL, 0, cb) )
	{
		switch( result )
		{
		case OV_EREAD: throw std::runtime_error("A read from media returned an error");
		case OV_ENOTVORBIS: throw std::runtime_error("Bitstream does not contain any Vorbis data");
		case OV_EVERSION: throw std::runtime_error("Vorbis version mismatch");
		case OV_EBADHEADER: throw std::runtime_error("Invalid Vorbis bitstream header");
		case OV_EFAULT: throw std::runtime_error("Internal logic fault; indicates a bug or heap/stack corruption");
		}
		throw std::runtime_error("unknown error opening ov stream");
	}

	try
	{
		vorbis_info *pinfo = ov_info(&vf, -1);
		if( NULL == pinfo )
		{
			throw std::runtime_error("could not get info from ov stream");
		}


		pwf->wFormatTag       = WAVE_FORMAT_PCM;
		pwf->nChannels        = pinfo->channels;
		pwf->nSamplesPerSec   = pinfo->rate;
		pwf->nAvgBytesPerSec  = pinfo->rate * pinfo->channels * 2;
		pwf->nBlockAlign      = pinfo->channels * 2;
		pwf->wBitsPerSample   = 16;
		pwf->cbSize           = 0;

		size_t size = ov_pcm_total(&vf, -1) * pwf->nBlockAlign;
		data->resize(size);

		int    bitstream = 0;
		size_t total = 0;

		while( total < size )
		{
			long ret = ov_read(&vf, &data->at(total), size - total, 0, 2, 1, &bitstream);
			if( 0 == ret )
			{
				break; // eof
			}
			if( ret < 0 )
			{
				// error in stream
				switch( ret )
				{
				case OV_HOLE:
					throw std::runtime_error("garbage between pages, loss of sync followed by recapture, or a corrupt page");
				case OV_EBADLINK:
					throw std::runtime_error("invalid stream section or the requested link is corrupt");
				case OV_EINVAL:
					throw std::runtime_error("initial file headers couldn't be read or are corrupt");
				}
				throw std::runtime_error("unknown error in ov stream");
			}
			else
			{
				total += ret;
			}
		}
	}
	catch(...)
	{
		ov_clear(&vf);
		throw;
	}
	ov_clear(&vf);
}

void LoadOggVorbis(bool init, enumSoundTemplate sound, const char *filename)
{
	if( init )
	{
		try
		{
			WAVEFORMATEX wfe = {0};
			std::vector<char> data;
			ogg_load_vorbis(filename, &wfe, &data);

			HRESULT hr = g_soundManager->CreateFromMemory( &g_pSounds[sound],
				(BYTE *) &data[0], data.size(), &wfe,
				DSBCAPS_CTRLPAN|DSBCAPS_CTRLVOLUME|DSBCAPS_CTRLFREQUENCY, GUID_NULL);

			if( FAILED(hr) )
				throw std::runtime_error("failed to create sound buffer");
		}
		catch( std::exception &e )
		{
			throw std::runtime_error(std::string("failed to load '") + filename + "' - " + e.what());
		}
	}
	else
	{
		SAFE_DELETE(g_pSounds[sound]);
	}
}

// end of file
