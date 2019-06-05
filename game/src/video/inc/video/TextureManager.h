#pragma once
#include "RenderBase.h"
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
	float pxFrameWidth;
	float pxFrameHeight;
	float pxBorderSize;
	std::vector<FRECT> uvFrames;
};

struct SpriteDefinition
{
	std::string textureFilePath;
	std::string spriteName;

	// mandatory
	int xframes;
	int yframes;
	float border;
	vec2d scale;
	vec2d atlasOffset;

	// optional - default values are relative to image size cannot be resolved until image is loaded
	vec2d atlasSize;
	vec2d pivot;

	// flags
	bool magFilter : 1;
	bool hasPivotX : 1;
	bool hasPivotY : 1;
	bool hasSizeX : 1;
	bool hasSizeY : 1;
};

class TextureManager final
{
public:
	TextureManager(TextureManager&&) = default;
	explicit TextureManager(IRender &render);
	~TextureManager();

	int LoadPackage(IRender& render, FS::FileSystem& fs, const std::vector<SpriteDefinition> &definitions);
	void UnloadAllTextures(IRender& render) noexcept;

	size_t FindSprite(std::string_view name) const;
	const DEV_TEXTURE& GetDeviceTexture(size_t texIndex) const { return _logicalTextures[texIndex].second->id; }
	const LogicalTexture& GetSpriteInfo(size_t texIndex) const { return _logicalTextures[texIndex].first; }
	float GetFrameWidth(size_t texIndex, size_t /*frameIdx*/) const { return _logicalTextures[texIndex].first.pxFrameWidth; }
	float GetFrameHeight(size_t texIndex, size_t /*frameIdx*/) const { return _logicalTextures[texIndex].first.pxFrameHeight; }
	vec2d GetFrameSize(size_t texIndex) const { return vec2d{GetFrameWidth(texIndex, 0), GetFrameHeight(texIndex, 0)}; }
	float GetBorderSize(size_t texIndex) const { return _logicalTextures[texIndex].first.pxBorderSize; }
	unsigned int GetFrameCount(size_t texIndex) const { return static_cast<unsigned int>(_logicalTextures[texIndex].first.uvFrames.size()); }

	void GetTextureNames(std::vector<std::string> &names, const char *prefix) const;

	float GetCharHeight(size_t fontTexture) const;
	float GetCharWidth(size_t fontTexture) const;

private:
	struct TexDesc
	{
		DEV_TEXTURE id;
		int width;          // The Width Of The Entire Image.
		int height;         // The Height Of The Entire Image.
		int refCount;       // number of logical textures
	};

	std::list<TexDesc> _devTextures;
	std::map<std::string, std::list<TexDesc>::iterator, std::less<>> _mapPath_to_TexDescIter;
	std::map<std::string, size_t, std::less<>> _mapName_to_Index;// index in _logicalTextures
	std::vector<std::pair<LogicalTexture, std::list<TexDesc>::iterator>> _logicalTextures;

	std::list<TexDesc>::iterator LoadTexture(IRender& render, FS::FileSystem& fs, std::string_view fileName, bool magFilter);

	void CreateChecker(IRender& render); // Create checker texture without name and with index=0
};

std::vector<SpriteDefinition> ParsePackage(const std::string &packageName, std::shared_ptr<FS::MemMap> file, FS::FileSystem &fs);

std::vector<SpriteDefinition> ParseDirectory(const std::string &dirName, const std::string &texPrefix, FS::FileSystem &fs);
