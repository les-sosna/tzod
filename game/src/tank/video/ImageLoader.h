// ImageLoader.h

#pragma once

// forward declarations
class IFile;
struct TextureData;

///////////////////////////////////////////////////////////////////////////////
// ImageLoader interface class declaration

class ImageLoader
{
protected:
	virtual ~ImageLoader(void) = 0;

public:
	virtual bool Load(const SafePtr<IFile> &file) = 0;
	virtual bool IsLoaded() = 0;
	virtual const TextureData* GetData() = 0;
};

///////////////////////////////////////////////////////////////////////////////
// TgaImageLoader class declaration

class TgaImageLoader : public ImageLoader
{
	struct TGAHeader
	{
		BYTE header[12];			// TGA file header
	};
	struct TGA
	{
		BYTE  header[6];        // First 6 useful bytes from the header
		int   bytesPerPixel;    // Holds number of bytes per pixel used in the tga file
		int   imageSize;        // Used to store the image size when setting aside ram
		int   temp;             // temporary variable
		int   type;
		int   height;           // Height of image
		int   width;            // Width of image
		int   bpp;              // Bits per pixel
	};

	void LoadUncompressedTGA(const SafePtr<IFile> &file);
	void LoadCompressedTGA(const SafePtr<IFile> &file);

	TextureData *_texData;


public:
	TgaImageLoader(void);
	virtual ~TgaImageLoader(void);

	virtual bool Load(const SafePtr<IFile> &file);
	virtual bool IsLoaded();
	virtual const TextureData* GetData();
};


// end of file
