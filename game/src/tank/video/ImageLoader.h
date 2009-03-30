// ImageLoader.h

#pragma once

#include "RenderBase.h"

// forward declarations
namespace FS
{
	class File;
}

class TgaImage : public Image
{
	struct TGA
	{
		long  bytesPerPixel;     // Holds number of bytes per pixel used in the tga file
		long  imageSize;         // Used to store the image size when setting aside ram
		long  temp;              // temporary variable
		long  type;
	};

	void LoadUncompressedTGA(const SafePtr<FS::File> &file);
	void LoadCompressedTGA(const SafePtr<FS::File> &file);

	long  _height;
	long  _width;
	long  _bpp;
	std::vector<char> _data;

public:
	TgaImage(const void *data, unsigned long size);
	virtual ~TgaImage();

	// Image methods
	const void* GetData() const;
	long GetBpp() const;
	long GetWidth() const;
	long GetHeight() const;
};


// end of file
