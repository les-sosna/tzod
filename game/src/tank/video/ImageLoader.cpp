// ImageLoader.cpp

#include "stdafx.h"
#include "ImageLoader.h"

#include "fs/FileSystem.h"


ImageLoader::~ImageLoader(void)
{
}

///////////////////////////////////////////////////////////////////////////////

TgaImageLoader::TgaImageLoader(void)
{
	_texData = NULL;
}

TgaImageLoader::~TgaImageLoader(void)
{
	if( _texData )
	{
		_ASSERT(_texData->imageData);
		delete[] _texData->imageData;
		delete _texData;
		_texData = NULL;
	}
}

bool TgaImageLoader::Load(const SafePtr<IFile> &file)
{
	_ASSERT(!_texData);

	static const BYTE uTGAcompare[12] = {0,0,2, 0,0,0,0,0,0,0,0,0};	// Uncompressed TGA Header
	static const BYTE cTGAcompare[12] = {0,0,10,0,0,0,0,0,0,0,0,0};	// Compressed TGA Header

	_texData = new TextureData;
	_texData->imageData = NULL;

	try
	{
		TGAHeader   tgaheader; // TGA header
		if( !file->Read(&tgaheader, sizeof(TGAHeader)) )
			throw -1;

		if( memcmp(uTGAcompare, &tgaheader, sizeof(tgaheader)) == 0 )
			LoadUncompressedTGA(file);
		else
		if( memcmp(cTGAcompare, &tgaheader, sizeof(tgaheader)) == 0 )
			LoadCompressedTGA(file);
		else
			throw -1;


		//
		// flip vertical
		//

		int lenght = _texData->width * (_texData->bpp >> 3);
		for( int y = 0; y < _texData->height >> 1; y++ )
		{
			BYTE *data1 = (BYTE*) _texData->imageData + y * lenght;
			BYTE *data2 = (BYTE*) _texData->imageData + (_texData->height - y - 1) * lenght;
			for( int x = 0; x < lenght; x++ )
			{
				BYTE tmp = data1[x];
				data1[x] = data2[x];
				data2[x] = tmp;
			}
		}
	}
	catch(int)
	{
		if( _texData->imageData )
			delete[] _texData->imageData;
		delete _texData;
		_texData = NULL;
	}

	return NULL != _texData;
}

bool TgaImageLoader::IsLoaded()
{
	return NULL != _texData;
}

const TextureData* TgaImageLoader::GetData()
{
	return _texData;
}


// Load an uncompressed TGA (note, much of this code is based on NeHe's
void TgaImageLoader::LoadUncompressedTGA(const SafePtr<IFile> &file)
{
	_ASSERT(_texData);

	TGA tga;	// Read TGA header
	if( !file->Read(tga.header, sizeof(tga.header)) )
		throw -1;

	_texData->width  = tga.header[1] * 256 + tga.header[0];	// Determine The TGA Width	(highbyte*256+lowbyte)
	_texData->height = tga.header[3] * 256 + tga.header[2];	// Determine The TGA Height	(highbyte*256+lowbyte)
	_texData->bpp	 = tga.header[4];		// Determine the bits per pixel
	tga.width		= _texData->width;		// Copy width into local structure
	tga.height		= _texData->height;		// Copy height into local structure
	tga.bpp			= _texData->bpp;			// Copy BPP into local structure

	if( (_texData->width <= 0) || (_texData->height <= 0) ||
		((_texData->bpp != 24) && (_texData->bpp !=32)) ) // Make sure all information is valid
	{
		throw -1;
	}

	// Compute the number of BYTES per pixel
	tga.bytesPerPixel	= (tga.bpp / 8);

	// Compute the total amout ofmemory needed to store data
	tga.imageSize		= (tga.bytesPerPixel * tga.width * tga.height);

	// Allocate that much memory
	_texData->imageData	= new BYTE[tga.imageSize];

	// Attempt to read image data
	if( !file->Read(_texData->imageData, tga.imageSize) )
	{
		throw -1;
	}

	for( int cswap = 0; cswap < tga.imageSize; cswap += tga.bytesPerPixel )
	{
		BYTE tmp = _texData->imageData[cswap];
		_texData->imageData[cswap] = _texData->imageData[cswap + 2];
		_texData->imageData[cswap + 2] = tmp;
	}
}

// Load COMPRESSED TGAs
void TgaImageLoader::LoadCompressedTGA(const SafePtr<IFile> &file)
{
	_ASSERT(_texData);

	TGA tga;	// Attempt to read header
	if( !file->Read(tga.header, sizeof(tga.header)) )
		throw -1;

	_texData->width  = tga.header[1] * 256 + tga.header[0];  // Determine The TGA Width	(highbyte*256+lowbyte)
	_texData->height = tga.header[3] * 256 + tga.header[2];  // Determine The TGA Height (highbyte*256+lowbyte)
	_texData->bpp 	 = tga.header[4];                        // Determine Bits Per Pixel
	tga.width		= _texData->width;                       // Copy width to local structure
	tga.height		= _texData->height;                      // Copy width to local structure
	tga.bpp			= _texData->bpp;                         // Copy width to local structure

	if( (_texData->width <= 0) || (_texData->height <= 0) ||
		((_texData->bpp != 24) && (_texData->bpp !=32)))	//Make sure all texture info is ok
	{
		throw -1;
	}

	tga.bytesPerPixel	= (tga.bpp / 8);                                // Compute BYTES per pixel
	tga.imageSize		= (tga.bytesPerPixel * tga.width * tga.height);	// Compute amout of memory needed to store image
	_texData->imageData = new BYTE[tga.imageSize];                      // Allocate that much memory

	int  pixelcount	  = tga.height * tga.width;                     // Nuber of pixels in the image
	int  currentpixel = 0;                                          // Current pixel being read
	int  currentbyte  = 0;                                          // Current byte
	BYTE *colorbuffer = new BYTE[tga.bytesPerPixel];         // Storage for 1 pixel

	do
	{
		BYTE chunkheader = 0;										// Storage for "chunk" header
		if( !file->Read(&chunkheader, sizeof(BYTE)) )			    // Read in the 1 byte header
		{
			throw -1;
		}

		if( chunkheader < 128 )												// If the ehader is < 128, it means the that is the number of RAW color packets minus 1
		{																	// that follow the header
			chunkheader++;													// add 1 to get number of following color values
			for( short counter = 0; counter < chunkheader; counter++ )		// Read RAW color values
			{
				// Try to read 1 pixel
				if( !file->Read(colorbuffer, tga.bytesPerPixel) )
				{
					delete[] colorbuffer;
					throw -1;
				}
																			// write to memory
				_texData->imageData[currentbyte		] = colorbuffer[2];		// Flip R and B vcolor values around in the process
				_texData->imageData[currentbyte + 1	] = colorbuffer[1];
				_texData->imageData[currentbyte + 2	] = colorbuffer[0];

				if( 4 == tga.bytesPerPixel )								// if its a 32 bpp image
					_texData->imageData[currentbyte + 3] = colorbuffer[3];  // copy the 4th byte

				currentbyte += tga.bytesPerPixel;	// Increase thecurrent byte by the number of bytes per pixel
				currentpixel++;						// Increase current pixel by 1

				if( currentpixel > pixelcount )		// Make sure we havent read too many pixels
				{
					delete[] colorbuffer;
					throw -1;
				}
			}
		}
		else	// chunkheader > 128 RLE data, next color reapeated chunkheader - 127 times
		{
			chunkheader -= 127;		// Subteact 127 to get rid of the ID bit

			// Attempt to read following color values
			if( !file->Read(colorbuffer, tga.bytesPerPixel) )
			{
				delete[] colorbuffer;
				throw -1;
			}

			for( short counter = 0; counter < chunkheader; counter++ )			// copy the color into the image data as many times as dictated
			{																	// by the header
				_texData->imageData[currentbyte     ] = colorbuffer[2];			// switch R and B bytes areound while copying
				_texData->imageData[currentbyte + 1 ] = colorbuffer[1];
				_texData->imageData[currentbyte + 2 ] = colorbuffer[0];

				if( 4 == tga.bytesPerPixel )                                // if tga images is 32 bpp
					_texData->imageData[currentbyte + 3] = colorbuffer[3];  // copy 4th byte

				currentbyte += tga.bytesPerPixel;	// Increase current byte by the number of bytes per pixel
				currentpixel++;						// Increase pixel count by 1

				if( currentpixel > pixelcount )		// Make sure we havent written too many pixels
				{
					delete[] colorbuffer;
					throw -1;
				}
			}
		}
	} while(currentpixel < pixelcount);	// Loop while there are still pixels left

	delete[] colorbuffer;
}


// end of file
