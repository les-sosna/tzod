#pragma once
#include "SoundRender.h"
#include <vector>

namespace FS {
	class FileSystem;
}

class SoundRenderOAL : public SoundRender
{
public:
	explicit SoundRenderOAL(FS::FileSystem &fs);
	~SoundRenderOAL();

	// SoundRender
	void SetListenerPos(vec2d pos) override;
	std::unique_ptr<Sound> CreateLopped(SoundTemplate sound) override;
	void PlayOnce(SoundTemplate sound, vec2d pos) override;
	void Step() override;

private:
	OALInitHelper _initHelper;
	std::vector<ALuint> _sources;
	std::vector<ALuint> _buffers;
	void LoadBuffer(FS::FileSystem &fs, SoundTemplate st, const char *fileName);
};
