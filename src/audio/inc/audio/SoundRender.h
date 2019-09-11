#pragma once
#include "detail/FormatDesc.h"
#include <math/MyMath.h>
#include <memory>

enum class SoundTemplate;

struct Sound
{
	virtual void SetPos(vec2d pos) = 0;
	virtual void SetPlaying(bool playing) = 0;
	virtual void SetVolume(float volume) = 0;
	virtual void SetPitch(float pitch) = 0;
	virtual ~Sound() {}
};

struct SoundRender
{
	virtual std::unique_ptr<Sound> CreateLooped(SoundTemplate sound) = 0;
	virtual void LoadBuffer(SoundTemplate st, const void *data, size_t size, FormatDesc format) = 0;
	virtual void PlayOnce(SoundTemplate sound, vec2d pos) = 0;
	virtual void SetListenerPos(vec2d pos) = 0;
	virtual void Step() = 0;
	virtual ~SoundRender() {}
};
