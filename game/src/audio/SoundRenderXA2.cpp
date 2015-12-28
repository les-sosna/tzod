#include "inc/audio/SoundRenderXA2.h"
#include <ui/ConsoleBuffer.h>
#include <algorithm>
#include <Xaudio2.h>

#define OPSETID 1U

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

	class SoundImpl : public Sound
	{
	public:
		SoundImpl(std::unique_ptr<IXAudio2SourceVoice, VoiceDeleter> source)
			: _source(std::move(source))
			, _started(false)
		{
		}

		~SoundImpl() override
		{
		}

		void SetPos(vec2d pos) override
		{
			// TODO: implement
		}

		void SetPlaying(bool playing) override
		{
			if (playing != _started)
			{
				if (playing)
					_source->Start(0U, OPSETID);
				else
					_source->Stop(0U, OPSETID);
				_started = playing;
			}
		}

		void SetVolume(float volume) override
		{
			_source->SetVolume(volume, OPSETID);
		}

		void SetPitch(float pitch) override
		{
			_source->SetFrequencyRatio(pitch, OPSETID);
		}

	private:
		std::unique_ptr<IXAudio2SourceVoice, VoiceDeleter> _source;
		bool _started;
	};
}

void VoiceDeleter::operator()(IXAudio2Voice *voice)
{
	voice->DestroyVoice();
}

SoundRenderXA2::SoundRenderXA2(UI::ConsoleBuffer &logger)
	: _logger(logger)
	, _masteringVoice(nullptr)
{
	HRESULT hr;
	hr = XAudio2Create(_xa2.ReleaseAndGetAddressOf());
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to initialize XAudio2 engine");
	}

#ifndef NDEBUG
	XAUDIO2_DEBUG_CONFIGURATION debugConfiguration;
	debugConfiguration.TraceMask = ~0U;
	debugConfiguration.BreakMask = 0;
	debugConfiguration.LogThreadID = FALSE;
	debugConfiguration.LogFileline = FALSE;
	debugConfiguration.LogFunctionName = FALSE;
	debugConfiguration.LogTiming = FALSE;
	_xa2->SetDebugConfiguration(&debugConfiguration);
#endif

	IXAudio2MasteringVoice *masteringVoice = nullptr;
	hr = _xa2->CreateMasteringVoice(&masteringVoice);
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create XAudio2 mastering voice");
	}
	_masteringVoice.reset(masteringVoice);
}

SoundRenderXA2::~SoundRenderXA2()
{
}

void SoundRenderXA2::SetListenerPos(vec2d pos)
{
	// TODO: implement
}

static std::unique_ptr<IXAudio2SourceVoice, VoiceDeleter> CreateVoice(UI::ConsoleBuffer &logger, IXAudio2 *xa2, const WAVEFORMATEX &fmt)
{
	std::unique_ptr<IXAudio2SourceVoice, VoiceDeleter> result;
	IXAudio2SourceVoice *rawSourceVoice = nullptr;
	if (SUCCEEDED(xa2->CreateSourceVoice(&rawSourceVoice, &fmt)))
		result.reset(rawSourceVoice);
	else
		logger.WriteLine(1, "Failed to create source voice");
	return result;
}

std::unique_ptr<Sound> SoundRenderXA2::CreateLopped(SoundTemplate sound)
{
	if ((size_t)sound < _buffers.size() && !_buffers[sound].data.empty())
	{
		Buffer &buffer = _buffers[sound];
		if (auto sourceVoice = CreateVoice(_logger, _xa2.Get(), buffer.wf))
		{
			XAUDIO2_BUFFER xaBuffer = {
				XAUDIO2_END_OF_STREAM,
				buffer.data.size(),
				buffer.data.data(),
				0, // PlayBegin
				0, // PlayLength; 0 to play the whole buffer
				0, // LoopBegin
				0, // LoopLength; 0 to loop the entire buffer
				XAUDIO2_LOOP_INFINITE
			};

			if (FAILED(sourceVoice->SubmitSourceBuffer(&xaBuffer)))
				_logger.WriteLine(1, "Failed to submit source buffer");
			else
				return std::make_unique<SoundImpl>(std::move(sourceVoice));
		}
	}

	return std::make_unique<SoundDummy>();
}

void SoundRenderXA2::PlayOnce(SoundTemplate sound, vec2d pos)
{
	if ((size_t) sound < _buffers.size() && !_buffers[sound].data.empty())
	{
		Buffer &buffer = _buffers[sound];
		if (auto sourceVoice = CreateVoice(_logger, _xa2.Get(), buffer.wf))
		{
			XAUDIO2_BUFFER xaBuffer = { XAUDIO2_END_OF_STREAM, buffer.data.size(), buffer.data.data() };
			if (FAILED(sourceVoice->SubmitSourceBuffer(&xaBuffer)))
				_logger.WriteLine(1, "Failed to submit source buffer");
			else if (FAILED(sourceVoice->Start(0, OPSETID)))
				_logger.WriteLine(1, "Failed to start source voice");
			else
				_sources.push_back(std::move(sourceVoice));
		}
	}
}

void SoundRenderXA2::Step()
{
	if (FAILED(_xa2->CommitChanges(OPSETID)))
	{
		_logger.WriteLine(1, "Failed to commit changes to audio engine");
	}

	_sources.erase(std::remove_if(_sources.begin(), _sources.end(), [](decltype(_sources)::reference source)
	{
		XAUDIO2_VOICE_STATE voiceState;
		source->GetState(&voiceState, XAUDIO2_VOICE_NOSAMPLESPLAYED);
		return 0 == voiceState.BuffersQueued;
	}), _sources.end());
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

