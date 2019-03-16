#pragma once
#include "SoundRender.h"
#include "detail/AudioContextOAL.h"
#include <vector>
#include <al.h>

class SoundRenderOAL : public SoundRender
{
public:
	SoundRenderOAL();
	~SoundRenderOAL();

	// SoundRender
	std::unique_ptr<Sound> CreateLooped(SoundTemplate sound) override;
    void LoadBuffer(SoundTemplate st, const void *data, size_t size, FormatDesc format) override;
	void PlayOnce(SoundTemplate sound, vec2d pos) override;
    void SetListenerPos(vec2d pos) override;
	void Step() override;

private:
	OALInitHelper _initHelper;
	std::vector<ALuint> _sources;
	std::vector<ALuint> _buffers;
};
