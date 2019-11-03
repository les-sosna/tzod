#pragma once
#include <math/MyMath.h>
#include <cassert>

struct ImageView
{
	const void* pixels;
	int width;
	int height;
	int stride;
	unsigned int bpp;

	ImageView Slice(RectRB rect) const
	{
		assert(rect.left >= 0 && rect.top >= 0);
		assert(rect.right <= width && rect.bottom <= height);
		assert(rect.left <= rect.right && rect.top <= rect.bottom);
		ImageView slice;
		slice.pixels = reinterpret_cast<const std::byte*>(pixels) + rect.left * bpp / 8 + rect.top * stride;
		slice.width = WIDTH(rect);
		slice.height = HEIGHT(rect);
		slice.stride = stride;
		slice.bpp = bpp;
		return slice;
	}
};
