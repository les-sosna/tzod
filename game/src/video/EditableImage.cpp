#include "inc/video/EditableImage.h"
#include <cassert>

EditableImage::EditableImage(unsigned int width, unsigned int height)
	: _width(width)
	, _height(height)
	, _pixels(width * height)
{
	assert(!_pixels.empty());
}

void EditableImage::Blit(RectRB dstRect, int dstGutters, int srcX, int srcY, const Image& source)
{
	assert(source.GetBpp() == 32);
	assert(dstRect.left - dstGutters >= 0 && dstRect.top - dstGutters >= 0);
	assert(dstRect.right + dstGutters <= _width && dstRect.bottom + dstGutters <= _height);
	assert(srcX + WIDTH(dstRect) <= (int)source.GetWidth() && srcY + HEIGHT(dstRect) <= (int)source.GetHeight());
	assert(srcX >= 0 && srcY >= 0);

	int rowPixels = WIDTH(dstRect);
	int rowBytes = WIDTH(dstRect) * sizeof(uint32_t);
	int numRows = HEIGHT(dstRect);
	int srcRowPitch = source.GetWidth();

	auto* src = static_cast<const uint32_t*>(source.GetData()) + srcY * srcRowPitch + srcX;
	auto* dst = _pixels.data() + (dstRect.top - dstGutters) * _width + dstRect.left;

	// top gutters
	for (int i = 0; i < dstGutters; i++)
	{
		memcpy(dst, src, rowBytes);
		for (int i = 0; i < dstGutters; i++)
		{
			dst[-1 - i] = src[0];
			dst[rowPixels + i] = src[rowPixels - 1];
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
			dst[rowPixels + i] = src[rowPixels - 1];
		}
		src += srcRowPitch;
		dst += _width;
	}

	// bottom gutters
	src -= srcRowPitch;
	for (int i = 0; i < dstGutters; i++)
	{
		memcpy(dst, src, rowBytes);
		for (int i = 0; i < dstGutters; i++)
		{
			dst[-1 - i] = src[0];
			dst[rowPixels + i] = src[rowPixels - 1];
		}
		dst += _width;
	}
}

const void* EditableImage::GetData() const
{
	return _pixels.data();
}

unsigned int EditableImage::GetBpp() const
{
	return 32;
}

unsigned int EditableImage::GetWidth() const
{
	return _width;
}

unsigned int EditableImage::GetHeight() const
{
	return _height;
}
