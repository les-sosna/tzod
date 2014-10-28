#include "SoundRender.h"
#include "globals.h" // TODO: remove
#include <iterator>
#include <utility>

class SoundDummy : public Sound
{
public:
	virtual void SetPos(vec2d) {}
	virtual void SetPlaying(bool) {}
	virtual void SetVolume(float) {}
	virtual void SetPitch(float) {}
};

class SoundImpl : public Sound
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

	virtual void SetPos(vec2d pos) override
	{
		alSource3f(_source, AL_POSITION, pos.x, pos.y, 0.0f);
	}
	
	virtual void SetPlaying(bool playing) override
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
	
	virtual void SetVolume(float volume) override
	{
		alSourcef(_source, AL_GAIN, volume);
	}
	
	virtual void SetPitch(float pitch) override
	{
		alSourcef(_source, AL_PITCH, pitch /* * g_conf.sv_speed.GetFloat() * 0.01f*/);
	}
	
private:
	ALuint _source;
};


SoundRender::SoundRender()
{
}

SoundRender::~SoundRender()
{
	if (_sources.empty())
	{
		alDeleteSources((ALsizei) _sources.size(), &_sources[0]);
	}
}

std::unique_ptr<Sound> SoundRender::CreateLopped(enumSoundTemplate sound)
{
	ALuint source = 0;
	alGenSources(1, &source);
	if (AL_NO_ERROR == alGetError())
	{
		alSourcei(source, AL_BUFFER, g_sounds[sound]);
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

void SoundRender::PlayOnce(enumSoundTemplate sound, vec2d pos)
{
	ALuint source = 0;
	alGenSources(1, &source);
	if (AL_NO_ERROR == alGetError())
	{
		_sources.push_back(source);
		alSourcei(source, AL_BUFFER, g_sounds[sound]);
		alSourcei(source, AL_REFERENCE_DISTANCE, 70);
		alSourcei(source, AL_LOOPING, AL_FALSE);
		alSource3f(source, AL_POSITION, pos.x, pos.y, 0.0f);
		alSourcePlay(source);
	}
}

void SoundRender::Step()
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

