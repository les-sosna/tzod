#include "inc/audio/SoundRenderOAL.h"
#include "SoundTemplates.h"
#include <fs/FileSystem.h>
#include <iterator>
#include <utility>

namespace
{

class SoundDummy final
	: public Sound
{
public:
	virtual void SetPos(vec2d) {}
	virtual void SetPlaying(bool) {}
	virtual void SetVolume(float) {}
	virtual void SetPitch(float) {}
};

class SoundImpl final
	: public Sound
{
public:
	SoundImpl(ALuint source)
		: _source(source)
	{
	}
	
	virtual ~SoundImpl()
	{
		alDeleteSources(1, &_source);
	}

	void SetPos(vec2d pos) override
	{
		alSource3f(_source, AL_POSITION, pos.x, pos.y, 0.0f);
	}
	
	void SetPlaying(bool playing) override
	{
		if (playing)
		{
			ALint state = 0;
			alGetSourcei(_source, AL_SOURCE_STATE, &state);
			if (AL_PLAYING != state) // do not restart if already playing
				alSourcePlay(_source);
		}
		else
		{
			alSourcePause(_source);
		}
	}
	
	void SetVolume(float volume) override
	{
		alSourcef(_source, AL_GAIN, volume);
	}
	
	void SetPitch(float pitch) override
	{
		alSourcef(_source, AL_PITCH, pitch /* * g_conf.sv_speed.GetFloat() * 0.01f*/);
	}
	
private:
	ALuint _source;
};

} // namespace

void SoundRenderOAL::LoadBuffer(SoundTemplate st, const void *data, size_t size, FormatDesc format)
{
    ALenum formatAL = format.channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;

    alBufferData(_buffers[static_cast<unsigned int>(st)], formatAL, data, size, format.frequency);
    ALenum e = alGetError();
    if (AL_NO_ERROR != e)
    {
        const ALchar *msg = alGetString(e);
        throw std::runtime_error(std::string("failed to fill sound buffer with data: ") +
                                 (msg ? msg : "Unknown OpenAL error"));
    }
}

SoundRenderOAL::SoundRenderOAL()
	: _buffers(static_cast<unsigned int>(SoundTemplate::COUNT))
{
	alGenBuffers(_buffers.size(), &_buffers[0]);
	ALenum e = alGetError();
	if (AL_NO_ERROR != e)
	{
		const ALchar *msg = alGetString(e);
		throw std::runtime_error(std::string("failed to create sound buffers: ") +
								 (msg ? msg : "Unknown OpenAL error"));
	}
}

SoundRenderOAL::~SoundRenderOAL()
{
	if (!_sources.empty())
	{
		alDeleteSources((ALsizei) _sources.size(), &_sources[0]);
	}
}

void SoundRenderOAL::SetListenerPos(vec2d pos)
{
    alListener3f(AL_POSITION, pos.x, pos.y, 500.0f);
}

std::unique_ptr<Sound> SoundRenderOAL::CreateLopped(SoundTemplate sound)
{
	ALuint source = 0;
	alGenSources(1, &source);
	if (AL_NO_ERROR == alGetError())
	{
		alSourcei(source, AL_BUFFER, _buffers[static_cast<unsigned int>(sound)]);
		alSourcei(source, AL_REFERENCE_DISTANCE, 70);
		alSourcei(source, AL_LOOPING, AL_TRUE);
		
		try {
			return std::unique_ptr<Sound>(new SoundImpl(source));
		}
		catch(...)
		{
			alDeleteSources(1, &source);
			throw;
		}
	}
	return std::unique_ptr<SoundDummy>(new SoundDummy());
}

void SoundRenderOAL::PlayOnce(SoundTemplate sound, vec2d pos)
{
	ALuint source = 0;
	alGenSources(1, &source);
	if (AL_NO_ERROR == alGetError())
	{
		_sources.push_back(source);
		alSourcei(source, AL_BUFFER, _buffers[static_cast<unsigned int>(sound)]);
		alSourcei(source, AL_REFERENCE_DISTANCE, 70);
		alSourcei(source, AL_LOOPING, AL_FALSE);
		alSource3f(source, AL_POSITION, pos.x, pos.y, 0.0f);
		alSourcePlay(source);
	}
}

void SoundRenderOAL::Step()
{
	auto finished = _sources.end();
	for (auto it = _sources.begin(); it != finished; )
	{
		ALint state = 0;
		alGetSourcei(*it, AL_SOURCE_STATE, &state);
		if( state != AL_PLAYING )
			std::swap(*it, *--finished);
		else
			++it;
	}
	
	if (finished != _sources.end())
	{
		ALsizei count = (ALsizei) std::distance(finished, _sources.end());
		alDeleteSources(count, &*finished);
		_sources.erase(finished, _sources.end());
	}
}
