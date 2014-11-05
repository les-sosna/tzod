#pragma once

#include <alc.h>

class OALInitHelper
{
public:
	OALInitHelper();
	~OALInitHelper();
	
private:
	ALCdevice *_device;
	ALCcontext *_context;
};
