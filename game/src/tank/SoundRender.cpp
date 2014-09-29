#include "SoundRender.h"
#include "globals.h" // TODO: remove
#include <iterator>
#include <utility>

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

