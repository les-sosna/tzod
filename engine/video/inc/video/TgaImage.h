#pragma once
#include "RenderBase.h"
#include <vector>

class TgaImage final
	: public Image
{
public:
	TgaImage(const void *data, unsigned long size);

	// Image
	ImageView GetData() const override;

private:
    unsigned int _width;
	unsigned int _height;
	unsigned int _bpp;
	std::vector<char> _data;
};

unsigned int GetTgaByteSize(ImageView image);
void WriteTga(ImageView image, void *dst, size_t bufferSize);

