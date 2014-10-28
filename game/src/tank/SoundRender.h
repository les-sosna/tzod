#pragma once
#include "SoundTemplates.h"
#include <math/MyMath.h>
#include <al.h>
#include <vector>

struct Sound
{
	virtual void SetPos(vec2d pos) = 0;
	virtual void SetPlaying(bool playing) = 0;
	virtual void SetVolume(float volume) = 0;
	virtual void SetPitch(float pitch) = 0;
	virtual ~Sound() {}
};

class SoundRender
{
public:
	SoundRender();
	~SoundRender();
	
	std::unique_ptr<Sound> CreateLopped(enumSoundTemplate sound);
	
	void PlayOnce(enumSoundTemplate sound, vec2d pos);
	void Step();
	
private:
	std::vector<ALuint> _sources;
};