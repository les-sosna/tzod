#pragma once
#include "SoundRender.h"
#include <vector>
#include <wrl\client.h>
#include <X3daudio.h>

struct IXAudio2;

class SoundRenderXA2 : public SoundRender
{
public:
	SoundRenderXA2();
	~SoundRenderXA2();

	// SoundRender
	std::unique_ptr<Sound> CreateLopped(SoundTemplate sound) override;
	void LoadBuffer(SoundTemplate st, const void *data, size_t size, FormatDesc format) override;
	void PlayOnce(SoundTemplate sound, vec2d pos) override;
	void SetListenerPos(vec2d pos) override;
	void Step() override;

private:
	Microsoft::WRL::ComPtr<IXAudio2> _xa2;
	X3DAUDIO_HANDLE _x3da;

	struct Buffer
	{
		WAVEFORMATEX wf;
		std::vector<BYTE> data;
	};
	std::vector<Buffer> _buffers;

	std::vector<int> _sources;
};

