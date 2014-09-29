#pragma once
#include "SoundTemplates.h"
#include <math/MyMath.h>
#include <al.h>
#include <vector>

class SoundRender
{
public:
	SoundRender();
	~SoundRender();
	
	void PlayOnce(enumSoundTemplate sound, vec2d pos);
	void Step();
	
private:
	std::vector<ALuint> _sources;
};