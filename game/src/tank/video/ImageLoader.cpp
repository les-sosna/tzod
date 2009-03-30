// ImageLoader.cpp

#include "stdafx.h"
#include "ImageLoader.h"

#include "fs/FileSystem.h"


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
	const Header &h = *(const Header *) data;

	_width  = h.header[1] * 256 + h.header[0];
	_height = h.header[3] * 256 + h.header[2];
	_bpp    = h.header[4];

	if( _width <= 0 || _height <= 0 || (_bpp != 24 && _bpp != 32) )
	{
		throw std::runtime_error("unsupported size or bpp");
	}

	long bytesPerPixel = _bpp / 8;
	long imageSize = bytesPerPixel * _width * _height;

	if( 0 == memcmp(signatureU, h.signature, 12) )
	{
		_data.assign(h.data, h.data + imageSize);
	}
	else if( 0 == memcmp(signatureC, h.signature, 12) )
	{
		_data.reserve(imageSize);

		int  pixelcount   = _height * _width;
		int  currentpixel = 0;
		int  currentbyte  = 0;

		// FIXME: check for buffer overflows
		do
		{
			const unsigned char &chunkheader = h.data[currentbyte++];

			if( chunkheader < 128 )    // If the header is < 128, it means the that is the number 
			{                          // of RAW color packets minus 1 that follow the header
				// Read RAW color values
				int pcount = chunkheader + 1;
				_data.insert(_data.end(), h.data + currentbyte, h.data + currentbyte + pcount * bytesPerPixel);
				currentpixel += pcount;
				currentbyte += pcount * bytesPerPixel;
			}
			else // chunkheader >= 128 RLE data, next color repeated chunkheader - 127 times
			{
				int pcount = chunkheader - 127;  // get rid of the ID bit

				const unsigned char *colorbuffer = h.data + currentbyte;
				currentbyte += bytesPerPixel;

				for( int counter = 0; counter < pcount; ++counter )
				{
					_data.insert(_data.end(), colorbuffer, colorbuffer + bytesPerPixel);
				}
				currentpixel += pcount;
			}
		} while(currentpixel < pixelcount); // Loop while there are still pixels left
	}
	else
	{
		throw std::runtime_error("unsupported TGA signature");
	}


	//
	// swap R <-> G
	//

	for( long cswap = 0; cswap < imageSize; cswap += bytesPerPixel )
	{
		std::swap(_data[cswap], _data[cswap + 2]);
	}


	//
	// flip vertical
	//

	long lenght = _width * (_bpp >> 3);
	for( long y = 0; y < _height >> 1; y++ )
	{
		char *data1 = &_data[y * lenght];
		char *data2 = &_data[(_height - y - 1) * lenght];
		for( long x = 0; x < lenght; ++x )
		{
			char tmp = data1[x];
			data1[x] = data2[x];
			data2[x] = tmp;
		}
	}
}

TgaImage::~TgaImage()
{
}

const void* TgaImage::GetData() const
{
	return &_data[0];
}

long TgaImage::GetBpp() const
{
	return _bpp;
}

long TgaImage::GetWidth() const
{
	return _width;
}

long TgaImage::GetHeight() const
{
	return _height;
}

/*
void TgaImageLoader::LoadCompressedTGA(const SafePtr<File> &file)
{
	_ASSERT(_texData);

	TGA tga;  // Attempt to read header
	if( !file->Read(tga.header, sizeof(tga.header)) )
		throw -1;

	_width  = tga.header[1] * 256 + tga.header[0];  // Determine The TGA Width (highbyte*256+lowbyte)
	_height = tga.header[3] * 256 + tga.header[2];  // Determine The TGA Height (highbyte*256+lowbyte)
	_bpp    = tga.header[4];                        // Determine Bits Per Pixel
	tga.width        = _texData->width;                      // Copy width to local structure
	tga.height       = _texData->height;                     // Copy width to local structure
	tga.bpp          = _texData->bpp;                        // Copy width to local structure

	if( (_texData->width <= 0) || (_texData->height <= 0) ||
		((_texData->bpp != 24) && (_texData->bpp !=32)))     //Make sure all texture info is ok
	{
		throw -1;
	}

	tga.bytesPerPixel = (tga.bpp / 8);                                // Compute BYTES per pixel
	tga.imageSize = (tga.bytesPerPixel * tga.width * tga.height); // Compute amout of memory needed to store image
	_texData->imageData = new BYTE[tga.imageSize];                      // Allocate that much memory

	int  pixelcount   = tga.height * tga.width;                     // Nuber of pixels in the image
	int  currentpixel = 0;                                          // Current pixel being read
	int  currentbyte  = 0;                                          // Current byte
	BYTE *colorbuffer = new BYTE[tga.bytesPerPixel];                // Storage for 1 pixel

	do
	{
		BYTE chunkheader = 0;                                       // Storage for "chunk" header
		if( !file->Read(&chunkheader, sizeof(BYTE)) )               // Read in the 1 byte header
		{
			throw -1;
		}

		if( chunkheader < 128 )        // If the ehader is < 128, it means the that is the number of RAW color packets minus 1
		{                              // that follow the header
			chunkheader++;             // add 1 to get number of following color values
			for( short counter = 0; counter < chunkheader; counter++ ) // Read RAW color values
			{
				// Try to read 1 pixel
				if( !file->Read(colorbuffer, tga.bytesPerPixel) )
				{
					delete[] colorbuffer;
					throw -1;
				}
				                                                         // write to memory
				_texData->imageData[currentbyte     ] = colorbuffer[2];  // Flip R and B vcolor values around in the process
				_texData->imageData[currentbyte + 1 ] = colorbuffer[1];
				_texData->imageData[currentbyte + 2 ] = colorbuffer[0];

				if( 4 == tga.bytesPerPixel )                                // if its a 32 bpp image
					_texData->imageData[currentbyte + 3] = colorbuffer[3];  // copy the 4th byte

				currentbyte += tga.bytesPerPixel;   // Increase thecurrent byte by the number of bytes per pixel
				currentpixel++;                     // Increase current pixel by 1

				if( currentpixel > pixelcount )     // Make sure we havent read too many pixels
				{
					delete[] colorbuffer;
					throw -1;
				}
			}
		}
		else // chunkheader > 128 RLE data, next color reapeated chunkheader - 127 times
		{
			chunkheader -= 127;  // Subteact 127 to get rid of the ID bit

			// Attempt to read following color values
			if( !file->Read(colorbuffer, tga.bytesPerPixel) )
			{
				delete[] colorbuffer;
				throw -1;
			}

			for( short counter = 0; counter < chunkheader; counter++ )   // copy the color into the image data as many times as dictated
			{                                                            // by the header
				_texData->imageData[currentbyte     ] = colorbuffer[2];  // switch R and B bytes areound while copying
				_texData->imageData[currentbyte + 1 ] = colorbuffer[1];
				_texData->imageData[currentbyte + 2 ] = colorbuffer[0];

				if( 4 == tga.bytesPerPixel )                                // if tga images is 32 bpp
					_texData->imageData[currentbyte + 3] = colorbuffer[3];  // copy 4th byte

				currentbyte += tga.bytesPerPixel;  // Increase current byte by the number of bytes per pixel
				currentpixel++;                    // Increase pixel count by 1

				if( currentpixel > pixelcount )    // Make sure we havent written too many pixels
				{
					delete[] colorbuffer;
					throw -1;
				}
			}
		}
	} while(currentpixel < pixelcount); // Loop while there are still pixels left

	delete[] colorbuffer;
}
*/

// end of file
