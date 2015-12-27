#include "inc/audio/SoundRenderXA2.h"
#include <Xaudio2.h>

namespace
{
	class SoundDummy : public Sound
	{
	public:
		virtual void SetPos(vec2d) {}
		virtual void SetPlaying(bool) {}
		virtual void SetVolume(float) {}
		virtual void SetPitch(float) {}
	};
}

SoundRenderXA2::SoundRenderXA2()
{
	HRESULT hr;
	hr = XAudio2Create(_xa2.ReleaseAndGetAddressOf());
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to initialize XAudio2 engine");
	}

	IXAudio2MasteringVoice *masteringVoice = nullptr;
	hr = _xa2->CreateMasteringVoice(&masteringVoice);
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create XAudio2 mastering voice");
	}
}

SoundRenderXA2::~SoundRenderXA2()
{
}

// SoundRender
void SoundRenderXA2::SetListenerPos(vec2d pos)
{
}

std::unique_ptr<Sound> SoundRenderXA2::CreateLopped(SoundTemplate sound)
{
	return std::make_unique<SoundDummy>();
}

void SoundRenderXA2::PlayOnce(SoundTemplate sound, vec2d pos)
{
	if ((size_t) sound < _buffers.size() && !_buffers[sound].data.empty())
	{
		IXAudio2SourceVoice *sourceVoice = nullptr;

		Buffer &buffer = _buffers[sound];
		HRESULT hr = _xa2->CreateSourceVoice(&sourceVoice, &buffer.wf);

		XAUDIO2_BUFFER xaBuffer = { XAUDIO2_END_OF_STREAM, buffer.data.size(), buffer.data.data() };
		sourceVoice->SubmitSourceBuffer(&xaBuffer);

		sourceVoice->Start();
	}
}

void SoundRenderXA2::Step()
{
}

void SoundRenderXA2::LoadBuffer(SoundTemplate st, const void *data, size_t size, FormatDesc format)
{
	if ((size_t) st >= _buffers.size())
	{
		_buffers.resize(st + 1);
	}

	Buffer &buffer = _buffers[st];

	buffer.wf.wFormatTag = WAVE_FORMAT_PCM;
	buffer.wf.nChannels = format.channels;
	buffer.wf.nSamplesPerSec = format.frequency;
	buffer.wf.wBitsPerSample = 16;
	buffer.wf.nBlockAlign = format.channels * (buffer.wf.wBitsPerSample / 8);
	buffer.wf.nAvgBytesPerSec = buffer.wf.nBlockAlign * format.frequency;
	buffer.wf.cbSize = 0; // Size of extra format information

	buffer.data.assign((const char *)data, (const char *)data + size);
}

