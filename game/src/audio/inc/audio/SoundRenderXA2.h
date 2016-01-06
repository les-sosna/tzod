#pragma once
#include "SoundRender.h"
#include <vector>
#include <Windows.h>
#include <mmreg.h>
#include <wrl\client.h>
//#include <X3daudio.h>

struct IXAudio2;
struct IXAudio2Voice;
struct IXAudio2SourceVoice;
struct IXAudio2MasteringVoice;

namespace UI
{
	class ConsoleBuffer;
}

struct VoiceDeleter
{
	void operator()(IXAudio2Voice *voice);
};

class SoundRenderXA2 : public SoundRender
{
public:
	explicit SoundRenderXA2(UI::ConsoleBuffer &logger);
	~SoundRenderXA2();

	// SoundRender
	std::unique_ptr<Sound> CreateLopped(SoundTemplate st) override;
	void LoadBuffer(SoundTemplate st, const void *data, size_t size, FormatDesc format) override;
	void PlayOnce(SoundTemplate st, vec2d pos) override;
	void SetListenerPos(vec2d pos) override;
	void Step() override;

private:
	UI::ConsoleBuffer &_logger;

	struct Buffer
	{
		WAVEFORMATEX wf;
		std::vector<BYTE> data;
	};
	std::vector<Buffer> _buffers;

	Microsoft::WRL::ComPtr<IXAudio2> _xa2;
	std::unique_ptr<IXAudio2MasteringVoice, VoiceDeleter> _masteringVoice;
	std::vector<std::unique_ptr<IXAudio2SourceVoice, VoiceDeleter>> _sources;

//	X3DAUDIO_HANDLE _x3da;
};

