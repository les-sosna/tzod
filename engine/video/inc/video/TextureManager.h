#pragma once
#include "ImageView.h"
#include "TexturePackage.h"
#include <list>
#include <map>
#include <memory>
#include <vector>
#include <string>

namespace FS
{
	struct MemMap;
	class FileSystem;
}

struct LogicalTexture
{
	vec2d pxPivot;
	float pxFrameWidth; // render size with border
	float pxFrameHeight;
	float pxBorderSize;
	int leadChar;
	int frameCount;
	bool magFilter;
	bool wrappable;
};

struct SpriteSource
{
	std::string textureFilePath;
	int srcX;
	int srcY;
	int pxFrameWidth;
	int pxFrameHeight;
	int xframes;
	int yframes;
};

class ImageCache final
{
public:
	ImageCache();
	~ImageCache();
	ImageView GetImage(FS::FileSystem& fs, std::string_view filePath);

private:
	std::map<std::string, class TgaImage, std::less<>> _loadedImages;
};

class TextureManager final
{
public:
	TextureManager(TextureManager&&) = default;
	TextureManager();
	~TextureManager();

	void LoadPackage(FS::FileSystem& fs, ImageCache& imageCache, const std::vector<PackageSpriteDesc>& packageSpriteDescs);
	void UnloadAllTextures() noexcept;

	size_t FindSprite(std::string_view name) const;
	const LogicalTexture& GetSpriteInfo(size_t texIndex) const { return _logicalTextures[texIndex]; }
	float GetFrameWidth(size_t texIndex, size_t /*frameIdx*/) const { return _logicalTextures[texIndex].pxFrameWidth; }
	float GetFrameHeight(size_t texIndex, size_t /*frameIdx*/) const { return _logicalTextures[texIndex].pxFrameHeight; }
	vec2d GetFrameSize(size_t texIndex) const { return vec2d{GetFrameWidth(texIndex, 0), GetFrameHeight(texIndex, 0)}; }
	float GetBorderSize(size_t texIndex) const { return _logicalTextures[texIndex].pxBorderSize; }
	int GetFrameCount(size_t texIndex) const { return _logicalTextures[texIndex].frameCount; }

	void GetTextureNames(std::vector<std::string> &names, const char *prefix) const;

	float GetCharHeight(size_t fontTexture) const;
	float GetCharWidth(size_t fontTexture) const;

	ImageView GetSpritePixels(FS::FileSystem& fs, ImageCache& imageCache, size_t texIndex, int frameIdx) const;
	size_t GetNextSprite(size_t texIndex) const;
	int GetVersion() const { return _version; }

private:
	std::map<std::string, size_t, std::less<>> _mapName_to_Index; // index in _logicalTextures and _spriteSources
	std::vector<LogicalTexture> _logicalTextures;
	std::vector<SpriteSource> _spriteSources;
	int _version = 0;
};
