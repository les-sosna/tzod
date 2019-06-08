#include "inc/video/EditableImage.h"
#include <cassert>

static constexpr int pixelBytes = 4;

EditableImage::EditableImage(unsigned int width, unsigned int height)
	: _width(width)
	, _height(height)
	, _data(width * height * pixelBytes)
{
	assert(!_data.empty());
}

void EditableImage::Blit(RectRB dstRect, int srcX, int srcY, const Image& source)
{
	assert(source.GetBpp() == 32);
	assert(dstRect.left >= 0 && dstRect.top >= 0 && dstRect.right <= _width && dstRect.bottom <= _height);
	assert(srcX + WIDTH(dstRect) <= (int)source.GetWidth() && srcY + HEIGHT(dstRect) <= (int)source.GetHeight());
	assert(srcX >= 0 && srcY >= 0);

	int rowBytes = WIDTH(dstRect) * pixelBytes;
	int numRows = HEIGHT(dstRect);
	int srcRowPitch = source.GetWidth() * pixelBytes;
	int dstRowPitch = _width * pixelBytes;

	const char* src = static_cast<const char*>(source.GetData()) + srcY * srcRowPitch + srcX * pixelBytes;
	char* dst = _data.data() + dstRect.top * dstRowPitch + dstRect.left * pixelBytes;

	for (int row = 0; row < numRows; row++)
	{
		memcpy(dst, src, rowBytes);
		src += srcRowPitch;
		dst += dstRowPitch;
	}
}

const void* EditableImage::GetData() const
{
	return _data.data();
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
