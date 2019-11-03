#include "inc/video/EditableImage.h"
#include <cassert>
#include <cstring>

EditableImage::EditableImage(unsigned int width, unsigned int height)
	: _width(width)
	, _height(height)
	, _pixels(width * height)
{
	assert(!_pixels.empty());
}

void EditableImage::Blit(int dstX, int dstY, int dstGutters, ImageView source)
{
	assert(source.bpp == 32);
	assert(dstX - dstGutters >= 0 && dstY - dstGutters >= 0);
	assert(dstX + source.width + dstGutters <= _width && dstY + source.height + dstGutters <= _height);

	int rowBytes = source.width * sizeof(uint32_t);
	int numRows = source.height;

	auto* src = static_cast<const uint32_t*>(source.pixels);
	auto* dst = _pixels.data() + (dstY - dstGutters) * _width + dstX;

	// top gutters
	for (int i = 0; i < dstGutters; i++)
	{
		memcpy(dst, src, rowBytes);
		for (int i = 0; i < dstGutters; i++)
		{
			dst[-1 - i] = src[0];
			dst[source.width + i] = src[source.width - 1];
		}
		dst += _width;
	}

	// content
	for (int row = 0; row < numRows; row++)
	{
		memcpy(dst, src, rowBytes);
		for (int i = 0; i < dstGutters; i++)
		{
			dst[-1 - i] = src[0];
			dst[source.width + i] = src[source.width - 1];
		}
		reinterpret_cast<const std::byte*&>(src) += source.stride;
		dst += _width;
	}

	// bottom gutters
	reinterpret_cast<const std::byte*&>(src) -= source.stride;
	for (int i = 0; i < dstGutters; i++)
	{
		memcpy(dst, src, rowBytes);
		for (int i = 0; i < dstGutters; i++)
		{
			dst[-1 - i] = src[0];
			dst[source.width + i] = src[source.width - 1];
		}
		dst += _width;
	}
}

ImageView EditableImage::GetData() const
{
    ImageView result = {};
    result.pixels = _pixels.data();
    result.width = _width;
    result.height = _height;
    result.stride = _width * 4;
    result.bpp = 32;
    return result;
}
