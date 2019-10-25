#pragma once
#include "RenderBase.h"
#include <vector>
#include <cstdint>

class EditableImage final
	: public Image
{
public:
	EditableImage(unsigned int width, unsigned int height);

	void Blit(RectRB dstRect, int dstGutters, int sourceX, int sourceY, const Image& source);

	// Image
	const void* GetData() const override;
	unsigned int GetBpp() const override;
	unsigned int GetWidth() const override;
	unsigned int GetHeight() const override;

private:
	int _height;
	int _width;
	std::vector<uint32_t> _pixels;
};

