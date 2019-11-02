#pragma once
#include "RenderBase.h"
#include <vector>
#include <cstdint>

class EditableImage final
	: public Image
{
public:
	EditableImage(unsigned int width, unsigned int height);

	void Blit(int dstX, int dstY, int dstGutters, ImageView source);

	// Image
	ImageView GetData() const override;

private:
    int _width;
	int _height;
	std::vector<uint32_t> _pixels;
};

