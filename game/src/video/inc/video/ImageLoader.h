// ImageLoader.h

#pragma once

#include "RenderBase.h"

#include <vector>

class TgaImage : public Image
{
	unsigned int _height;
	unsigned int _width;
	unsigned int _bpp;
	std::vector<char> _data;

public:
	TgaImage(const void *data, unsigned long size);
	virtual ~TgaImage();

	// Image methods
	const void* GetData() const override;
	unsigned int GetBpp() const override;
	unsigned int GetWidth() const override;
	unsigned int GetHeight() const override;
};


// end of file
