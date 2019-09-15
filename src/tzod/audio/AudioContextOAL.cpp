#include "inc/audio/detail/AudioContextOAL.h"
#include <stdexcept>

OALInitHelper::OALInitHelper()
	: _device(nullptr)
	, _context(nullptr)
{
	_device = alcOpenDevice(nullptr);
	if (!_device)
	{
		throw std::runtime_error("failed to open audio device");
	}
	
	_context = alcCreateContext(_device, nullptr);
	if (!_context)
	{
		alcCloseDevice(_device);
		throw std::runtime_error("failed to create audio context");
	}
	
	alcMakeContextCurrent(_context);
}
	
OALInitHelper::~OALInitHelper()
{
	alcMakeContextCurrent(nullptr);
	alcDestroyContext(_context);
	alcCloseDevice(_device);
}
