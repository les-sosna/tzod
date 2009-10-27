// MusicPlayer.cpp

#include "stdafx.h"
#include "MusicPlayer.h"

#include "macros.h"

#include "fs/FileSystem.h"
#include "config/Config.h"



MusicPlayer::MusicPlayer()
  : _looping(true)
  , _playbackDone(false)
  , _firstHalfPlaying(true)
  , _bufHalfSize(0)
{
	ZeroMemory(&_vorbisFile, sizeof(OggVorbis_File));
	ZeroMemory(&_state, sizeof(State));
	g_conf.s_musicvolume.eventChange = std::tr1::bind(&MusicPlayer::OnChangeVolume, this);
}

MusicPlayer::~MusicPlayer()
{
	g_conf.s_musicvolume.eventChange = NULL;
	Cleanup();
}

void MusicPlayer::OnChangeVolume()
{
	if( _buffer )
	{
		_buffer->SetVolume(DSBVOLUME_MIN + int((float) (g_conf.s_musicvolume.GetInt() - DSBVOLUME_MIN)));
	}
}

void MusicPlayer::Cleanup()
{
	_buffer.Release();
	_bufHalfSize = 0;
	ov_clear(&_vorbisFile);
	_state.file = NULL;
	_state.ptr = 0;
}

bool MusicPlayer::Fill(bool firstHalf)
{
	LPVOID firstSegment;
	DWORD  firstSegmentSize = 0;
	LPVOID secondSegment;
	DWORD  secondSegmentSize = 0;

	if( FAILED(_buffer->Lock((firstHalf ? 0 : _bufHalfSize), _bufHalfSize,
	                         &firstSegment, &firstSegmentSize, &secondSegment, &secondSegmentSize, 0)) )
	{
		return false;
	}

	assert(firstSegmentSize == _bufHalfSize);
	

	//
	// decode OGG file into buffer
	//

	unsigned int nBytesReadSoFar  = 0; // keep track of how many bytes we have read so far
	long nBytesReadThisTime       = 1; // keep track of how many bytes we read per ov_read invokation (1 to ensure that while loop is entered below)
	int nBitStream                = 0; // used to specify logical bitstream 0

	// decode vorbis file into buffer half (continue as long as the buffer hasn't been filled with something (=sound/silence)
	while( nBytesReadSoFar < _bufHalfSize )
	{
		nBytesReadThisTime = ov_read(
			&_vorbisFile,
			(char*) firstSegment + nBytesReadSoFar, // where to put the decoded data
			_bufHalfSize - nBytesReadSoFar,         // how much data to read
			0,                                      // 0 specifies little endian decoding mode
			2,                                      // 2 specifies 16-bit samples
			1,                                      // 1 specifies signed data
			&nBitStream
		);

		//new position corresponds to the amount of data we just read
		nBytesReadSoFar += nBytesReadThisTime;


		//
		// do special processing if we have reached end of the OGG file
		//

		if( 0 == nBytesReadThisTime )
		{
			// if looping we fill start of OGG, otherwise fill with silence
			if( _looping )
			{
				// seek back to beginning of file
				ov_time_seek(&_vorbisFile, 0);
			}
			else
			{
				// fill with silence
				for( unsigned int i = nBytesReadSoFar; i < _bufHalfSize; i++)
				{
					// silence = 0 in 16 bit sampled data (which OGG always is)
					*((char*) firstSegment + i) = 0;
				}

				// signal that playback is over and exit the reader loop
				_playbackDone = true;
				nBytesReadSoFar = _bufHalfSize;
			}
		}
	}

	_buffer->Unlock(firstSegment, firstSegmentSize, secondSegment, secondSegmentSize);
	return true;
}

size_t MusicPlayer::read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	State *s = (State *) datasource;
	assert(s->ptr <= s->file->GetSize());
	size_t rd = __min(s->file->GetSize() - s->ptr, size*nmemb);
	memcpy(ptr, s->file->GetData() + s->ptr, rd);
	s->ptr += rd;
	return rd;
}

int MusicPlayer::seek_func(void *datasource, ogg_int64_t offset, int whence)
{
	State *s = (State *) datasource;
	switch( whence )
	{
	case SEEK_CUR:
		s->ptr += offset;
		break;
	case SEEK_END:
		s->ptr = s->file->GetSize() - offset;
		break;
	case SEEK_SET:
		s->ptr = offset;
	}
	return 0; // TODO: validate input
}

long MusicPlayer::tell_func(void *datasource)
{
	return ((State *) datasource)->ptr;
}

bool MusicPlayer::Load(SafePtr<FS::MemMap> file)
{
	Cleanup();

	_state.file = file; // hold reference to file
	_state.ptr = 0;

	//open the file as an OGG file (allocates internal OGG buffers)
	ov_callbacks cb;
	cb.read_func  = &MusicPlayer::read_func;
	cb.seek_func  = &MusicPlayer::seek_func;
	cb.close_func = NULL;
	cb.tell_func  = &MusicPlayer::tell_func;
	
	if( (ov_open_callbacks(&_state, &_vorbisFile, NULL, 0, cb)) != 0 )
	{
		return false;
	}


	// get info
	vorbis_info *vorbisInfo = ov_info(&_vorbisFile, -1);


	//
	// setup buffer
	//

	WAVEFORMATEX wf = {0};
	wf.wFormatTag       = 1;   // always 1 in OGG
	wf.nChannels        = vorbisInfo->channels;
	wf.nSamplesPerSec   = vorbisInfo->rate;
	wf.nAvgBytesPerSec  = wf.nSamplesPerSec * wf.nChannels * 2;
	wf.nBlockAlign      = 2 * wf.nChannels;
	wf.wBitsPerSample   = 16;  // always 16 in OGG
	wf.cbSize           = 0;   // no extra info

	_bufHalfSize = __min(10000, __max(100, g_conf.s_buffer.GetInt())) * wf.nAvgBytesPerSec / 2000;

	DSBUFFERDESC desc = {0};
	desc.dwSize         = sizeof(DSBUFFERDESC);
	desc.lpwfxFormat    = &wf;
	desc.dwBufferBytes  = _bufHalfSize * 2;
	desc.dwFlags        = DSBCAPS_CTRLVOLUME;

	//pointer to old interface, used to obtain pointer to new interface
	ComPtr<IDirectSoundBuffer> pTempBuffer;	
	if( FAILED(g_soundManager->GetDirectSound()->CreateSoundBuffer(&desc, &pTempBuffer, NULL)) )
	{
		_bufHalfSize = 0;
		return false;
	}

	//query for updated interface
	if( FAILED(pTempBuffer->QueryInterface(IID_IDirectSoundBuffer8, (void**)&_buffer)))
	{
		_bufHalfSize = 0;
		return false;
	}
	
	OnChangeVolume();

	Fill(true); // Fill first half of buffer with initial data
	_firstHalfPlaying = false;

	return true;
}

void MusicPlayer::Play(bool looping)
{
	_buffer->Play(0, 0, DSBPLAY_LOOPING);
}

void MusicPlayer::HandleBufferFilling()
{
	if( _buffer )
	{
		DWORD pos;
		if( SUCCEEDED(_buffer->GetCurrentPosition(&pos, NULL)) )
		{
			if( pos > _bufHalfSize && _firstHalfPlaying )
			{
				Fill(true); // fill first half
				_firstHalfPlaying = false;
			}
			else if( pos <= _bufHalfSize && !_firstHalfPlaying )
			{
				Fill(false); // fill second half
				_firstHalfPlaying = true;
			}
		}
	}
}

// end of file
