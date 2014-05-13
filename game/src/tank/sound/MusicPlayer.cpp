// MusicPlayer.cpp

#include "MusicPlayer.h"

#include "Macros.h"

#include "config/Config.h"
#include <fs/FileSystem.h>
#include <al.h>


static void ThrowIfALError()
{
    ALenum e = alGetError();
    if (AL_NO_ERROR != e)
    {
        const ALchar *msg = alGetString(e);
        throw std::runtime_error(msg ? msg : "Unknown OpenAL error");
    }
}

static void LogALError()
{
    ALenum e = alGetError();
    if (AL_NO_ERROR != e)
    {
        const ALchar *msg = alGetString(e);
        printf("%s\n", msg ? msg : "Unknown OpenAL error");
    }
}


MusicPlayer::MusicPlayer()
  : _playing(false)
{
	memset(&_vorbisFile, 0, sizeof(OggVorbis_File));
    alGenBuffers(_buffers.size(), _buffers.data());
    ThrowIfALError();
    alGenSources(1, &_source);
    ThrowIfALError();
	g_conf.s_musicvolume.eventChange = std::bind(&MusicPlayer::OnChangeVolume, this);
}

MusicPlayer::~MusicPlayer()
{
	g_conf.s_musicvolume.eventChange = NULL;
	Cleanup();
    alDeleteSources(1, &_source);
    LogALError();
    alDeleteBuffers(_buffers.size(), _buffers.data());
    LogALError();
}

void MusicPlayer::OnChangeVolume()
{
//	if( _buffer )
//	{
//		_buffer->SetVolume(DSBVOLUME_MIN + int((float) (g_conf.s_musicvolume.GetInt() - DSBVOLUME_MIN)));
//	}
}

void MusicPlayer::Cleanup()
{
	ov_clear(&_vorbisFile);
    _state.file.reset();
    _state.ptr = 0;
}

void MusicPlayer::FillAndQueue(ALuint bufName)
{
    char buf[16384];
    
	unsigned int nBytesRead  = 0;
	int nBitStream           = 0; // used to specify logical bitstream 0

	while( nBytesRead < sizeof(buf) )
	{
		long count = ov_read(
			&_vorbisFile,
			buf + nBytesRead,           // where to put the decoded data
			sizeof(buf) - nBytesRead,   // how much data to read
			0,                          // 0 specifies little endian decoding mode
			2,                          // 2 specifies 16-bit samples
			1,                          // 1 specifies signed data
			&nBitStream
		);

		nBytesRead += count;

        // seek back to the beginning if we reached the end of file
		if( 0 == count )
            ov_pcm_seek(&_vorbisFile, 0);
	}

    const vorbis_info *vi = ov_info(&_vorbisFile, -1);
    alBufferData(bufName, vi->channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, buf, sizeof(buf), vi->rate);
    ThrowIfALError();
    alSourceQueueBuffers(_source, 1, &bufName);
    ThrowIfALError();
}

size_t MusicPlayer::read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	State *s = (State *) datasource;
	assert(s->ptr <= s->file->GetSize());
	size_t rd = std::min(s->file->GetSize() - s->ptr, size*nmemb);
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

bool MusicPlayer::Load(std::shared_ptr<FS::MemMap> file)
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
        Cleanup();
		return false;
	}

	// get info
	vorbis_info *vorbisInfo = ov_info(&_vorbisFile, -1);
    if (vorbisInfo->channels != 1 && vorbisInfo->channels != 2)
    {
        Cleanup();
        return false;
    }

	OnChangeVolume();

    for (ALuint name: _buffers)
        FillAndQueue(name);

	return true;
}

void MusicPlayer::Play()
{
    _playing = true;
    alSourcePlay(_source);
    LogALError();
}

void MusicPlayer::HandleBufferFilling()
{
    if (_playing)
    {
        ALint value = 0;
        alGetSourcei(_source, AL_BUFFERS_PROCESSED, &value);
        if( value > 0 )
        {
            ALuint buffer = AL_NONE;
            alSourceUnqueueBuffers(_source, 1, &buffer);
            if (AL_NONE != buffer)
            {
                FillAndQueue(buffer);
                // restart if it previously run out of buffers
                alGetSourcei(_source, AL_SOURCE_STATE, &value);
                if( AL_PLAYING != value )
                    alSourcePlay(_source);
            }
        }
    }
}

// end of file
