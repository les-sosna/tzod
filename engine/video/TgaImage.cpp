#include "inc/video/TgaImage.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>

static const uint8_t signatureU[12] = { 0,0,2, 0,0,0,0,0,0,0,0,0 }; // Uncompressed
static const uint8_t signatureC[12] = { 0,0,10,0,0,0,0,0,0,0,0,0 }; // Compressed

namespace
{
	struct Header
	{
		uint8_t signature[12];
		uint16_t width;
		uint16_t height;
		uint8_t bpp;
		uint8_t descriptor;
		uint8_t data[1];
	};
}

TgaImage::TgaImage(const void *data, unsigned long size)
{
	if( size < sizeof(Header) )
	{
		throw std::runtime_error("corrupted TGA image");
	}

	const Header &h = *(const Header *) data;
	unsigned long dataSize = size - offsetof(Header, data);

	_width  = h.width;
	_height = h.height;
	_bpp    = h.bpp;

	if( _width <= 0 || _height <= 0 || (_bpp != 24 && _bpp != 32) )
	{
		throw std::runtime_error("unsupported size or bpp");
	}

	const unsigned int bytesPerPixel = _bpp / 8;
	const unsigned int imageSizeBytes = bytesPerPixel * _width * _height;
	const unsigned int pixelCount = _height * _width;

	if( 0 == memcmp(signatureU, h.signature, 12) )
	{
		if( dataSize < imageSizeBytes )
		{
			throw std::runtime_error("corrupted TGA image");
		}
		_data.assign(h.data, h.data + imageSizeBytes);
	}
	else if( 0 == memcmp(signatureC, h.signature, 12) )
	{
		unsigned int currentPixel = 0;
		unsigned int currentByte  = 0;
		_data.reserve(imageSizeBytes);
		do
		{
			if( currentByte >= dataSize )
			{
				throw std::runtime_error("corrupted TGA image");
			}
			const uint8_t chunkHeader = h.data[currentByte++];

			if( chunkHeader < 128 )    // If the header is < 128, it means the that is the number
			{                          // of RAW color packets minus 1 that follow the header
				// Read RAW color values
				int pcount = chunkHeader + 1;
				currentPixel += pcount;
				if( pixelCount < currentPixel || dataSize < currentByte + pcount * bytesPerPixel )
				{
					throw std::runtime_error("corrupted TGA image");
				}
				_data.insert(_data.end(), h.data + currentByte, h.data + currentByte + pcount * bytesPerPixel);
				currentByte += pcount * bytesPerPixel;
			}
			else // chunkHeader >= 128 RLE data, next color repeated chunkHeader - 127 times
			{
				int pcount = chunkHeader - 127;  // get rid of the ID bit
				currentPixel += pcount;
				if( pixelCount < currentPixel || dataSize < currentByte + bytesPerPixel  )
				{
					throw std::runtime_error("corrupted TGA image");
				}
				const uint8_t*colorBuffer = h.data + currentByte;
				currentByte += bytesPerPixel;
				for( int counter = 0; counter < pcount; ++counter )
				{
					_data.insert(_data.end(), colorBuffer, colorBuffer + bytesPerPixel);
				}
			}
		}
		while( currentPixel < pixelCount );
	}
	else
	{
		throw std::runtime_error("unsupported TGA signature");
	}

	// swap R <-> G
	for( unsigned int cswap = 0; cswap < imageSizeBytes; cswap += bytesPerPixel )
	{
		std::swap(_data[cswap], _data[cswap + 2]);
	}

	// flip vertical
	unsigned int rowSizeBytes = _width * bytesPerPixel;
	for( unsigned long y = 0; y < _height / 2; y++ )
	{
		std::swap_ranges(_data.begin() + y * rowSizeBytes,
			_data.begin() + y * rowSizeBytes + rowSizeBytes,
			_data.begin() + (_height - y - 1) * rowSizeBytes);
	}

	// convert to 32 bit
	if (3 == bytesPerPixel)
	{
		_data.resize(pixelCount * 4);
		for (unsigned int pixel = pixelCount; pixel--;)
		{
			*(uint32_t*)&_data[pixel * 4] = (*(uint32_t*)&_data[pixel * 3] & 0xffffff) | 0xff000000;
		}
		_bpp = 32;
	}
}

ImageView TgaImage::GetData() const
{
    ImageView result = {};
    result.pixels = _data.data();
    result.width = _width;
    result.height = _height;
    result.stride = _width * _bpp / 8;
    result.bpp = _bpp;
    return result;
}

///////////////////////////////////////////////////////////////////////////////

unsigned int GetTgaByteSize(ImageView image)
{
	assert(image.bpp == 32);
	return offsetof(Header, data) + image.width * image.height * 4;
}

void WriteTga(ImageView image, void* dst, size_t bufferSize)
{
	if (bufferSize < GetTgaByteSize(image))
	{
		throw std::runtime_error("Insufficient buffer size");
	}

	auto &header = *reinterpret_cast<Header*>(dst);
	memcpy(header.signature, signatureU, sizeof(signatureU));
	header.width = image.width;
	header.height = image.height;
	header.bpp = image.bpp;
	header.descriptor = 0;

	// flip row order and swap red & blue channels
	auto dstPixels = reinterpret_cast<uint32_t*>(header.data);
    auto srcBytes = reinterpret_cast<const std::byte*>(image.pixels);
	auto width = image.width;
	auto height = image.height;
	for (unsigned int i = 0; i < height; ++i)
	{
		auto dstRow = dstPixels + i * width;
		auto srcRow = reinterpret_cast<const uint32_t*>(srcBytes + (height - i - 1) * image.stride);
		for (unsigned int j = 0; j < width; ++j)
		{
			auto c = srcRow[j];
			dstRow[j] = ((c & 0xff) << 16) | (c & 0xff00ff00) | ((c & 0xff0000) >> 16);
		}
	}
}

