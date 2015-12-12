#include "inc/video/ImageLoader.h"
#include <algorithm>
#include <cstddef>
#include <cstring>

TgaImage::TgaImage(const void *data, unsigned long size)
{
	static const unsigned char signatureU[12] = {0,0,2, 0,0,0,0,0,0,0,0,0}; // Uncompressed
	static const unsigned char signatureC[12] = {0,0,10,0,0,0,0,0,0,0,0,0}; // Compressed

	struct Header
	{
		unsigned char signature[12];
		unsigned char header[6]; // First 6 useful bytes from the header
		unsigned char data[1];
	};

	if( size < sizeof(Header) )
	{
		throw std::runtime_error("corrupted TGA image");
	}

	const Header &h = *(const Header *) data;
	unsigned long dataSize = size - offsetof(Header, data);

	_width  = h.header[1] * 256 + h.header[0];
	_height = h.header[3] * 256 + h.header[2];
	_bpp    = h.header[4];

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
			const unsigned char chunkHeader = h.data[currentByte++];

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
				const unsigned char *colorBuffer = h.data + currentByte;
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

TgaImage::~TgaImage()
{
}

const void* TgaImage::GetData() const
{
	return &_data[0];
}

unsigned int TgaImage::GetBpp() const
{
	return _bpp;
}

unsigned int TgaImage::GetWidth() const
{
	return _width;
}

unsigned int TgaImage::GetHeight() const
{
	return _height;
}
