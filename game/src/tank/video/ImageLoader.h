// ImageLoader.h

#pragma once

#include "RenderBase.h"

class TgaImage : public Image
{
	unsigned long _height;
	unsigned long _width;
	unsigned long _bpp;
	std::vector<char> _data;

public:
	TgaImage(const void *data, unsigned long size);
	virtual ~TgaImage();

	// Image methods
	const void* GetData() const;
	unsigned long GetBpp() const;
	unsigned long GetWidth() const;
	unsigned long GetHeight() const;
};


// end of file
