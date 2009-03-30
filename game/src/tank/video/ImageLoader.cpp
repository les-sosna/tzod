// ImageLoader.cpp

#include "stdafx.h"
#include "ImageLoader.h"

///////////////////////////////////////////////////////////////////////////////

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

	long bytesPerPixel = _bpp / 8;
	unsigned long imageSize = bytesPerPixel * _width * _height;

	if( 0 == memcmp(signatureU, h.signature, 12) )
	{
		if( dataSize < imageSize )
		{
			throw std::runtime_error("corrupted TGA image");
		}
		_data.assign(h.data, h.data + imageSize);
	}
	else if( 0 == memcmp(signatureC, h.signature, 12) )
	{
		unsigned long pixelcount   = _height * _width;
		unsigned long currentpixel = 0;
		unsigned long currentbyte  = 0;
		_data.reserve(imageSize);
		do
		{
			if( currentbyte >= dataSize )
			{
				throw std::runtime_error("corrupted TGA image");
			}
			const unsigned char &chunkheader = h.data[currentbyte++];

			if( chunkheader < 128 )    // If the header is < 128, it means the that is the number 
			{                          // of RAW color packets minus 1 that follow the header
				// Read RAW color values
				int pcount = chunkheader + 1;
				currentpixel += pcount;
				if( pixelcount < currentpixel || dataSize < currentbyte + pcount * bytesPerPixel )
				{
					throw std::runtime_error("corrupted TGA image");
				}
				_data.insert(_data.end(), h.data + currentbyte, h.data + currentbyte + pcount * bytesPerPixel);
				currentbyte += pcount * bytesPerPixel;
			}
			else // chunkheader >= 128 RLE data, next color repeated chunkheader - 127 times
			{
				int pcount = chunkheader - 127;  // get rid of the ID bit
				currentpixel += pcount;
				if( pixelcount < currentpixel || dataSize < currentbyte + bytesPerPixel  )
				{
					throw std::runtime_error("corrupted TGA image");
				}
				const unsigned char *colorbuffer = h.data + currentbyte;
				currentbyte += bytesPerPixel;
				for( int counter = 0; counter < pcount; ++counter )
				{
					_data.insert(_data.end(), colorbuffer, colorbuffer + bytesPerPixel);
				}
			}
		}
		while( currentpixel < pixelcount );
	}
	else
	{
		throw std::runtime_error("unsupported TGA signature");
	}

	// swap R <-> G
	for( unsigned long cswap = 0; cswap < imageSize; cswap += bytesPerPixel )
	{
		std::swap(_data[cswap], _data[cswap + 2]);
	}

	// flip vertical
	unsigned long len = _width * (_bpp >> 3);
	for( unsigned long y = 0; y < _height >> 1; y++ )
	{
		std::swap_ranges(_data.begin() + y * len, _data.begin() + y * len + len,
			_data.begin() + (_height - y - 1) * len);
	}
}

TgaImage::~TgaImage()
{
}

const void* TgaImage::GetData() const
{
	return &_data[0];
}

unsigned long TgaImage::GetBpp() const
{
	return _bpp;
}

unsigned long TgaImage::GetWidth() const
{
	return _width;
}

unsigned long TgaImage::GetHeight() const
{
	return _height;
}

// end of file
