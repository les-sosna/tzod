#pragma once
#include "AudioContext.h"
#include "SoundTemplates.h"
#include <math/MyMath.h>
#include <al.h>
#include <vector>

namespace FS {
	class FileSystem;
}

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
	explicit SoundRender(FS::FileSystem &fs);
	~SoundRender();
	
	std::unique_ptr<Sound> CreateLopped(SoundTemplate sound);
	
	void PlayOnce(SoundTemplate sound, vec2d pos);
	void Step();
	
private:
	OALInitHelper _initHelper;
	std::vector<ALuint> _sources;
	std::vector<ALuint> _buffers;
	void LoadBuffer(FS::FileSystem &fs, SoundTemplate st, const char *fileName);
};
