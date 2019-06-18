#pragma once
#include "RenderBase.h"
#include <vector>

class TgaImage final
	: public Image
{
public:
	TgaImage(const void *data, unsigned long size);

	// Image
	const void* GetData() const override;
	unsigned int GetBpp() const override;
	unsigned int GetWidth() const override;
	unsigned int GetHeight() const override;

private:
	unsigned int _height;
	unsigned int _width;
	unsigned int _bpp;
	std::vector<char> _data;
};

unsigned int GetTgaByteSize(const Image& image);
void WriteTga(const Image& image, void *dst, size_t bufferSize);

