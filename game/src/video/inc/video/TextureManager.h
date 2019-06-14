#pragma once
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
	float pxFrameWidth;
	float pxFrameHeight;
	float pxBorderSize;
	int leadChar;
	std::vector<FRECT> uvFrames;
};

class TextureManager final
{
public:
	TextureManager(TextureManager&&) = default;
	TextureManager();
	~TextureManager();

	void LoadPackage(FS::FileSystem& fs, const std::vector<PackageSpriteDesc>& packageSpriteDescs);
	void UnloadAllTextures() noexcept;

	size_t FindSprite(std::string_view name) const;
	const LogicalTexture& GetSpriteInfo(size_t texIndex) const { return _logicalTextures[texIndex]; }
	float GetFrameWidth(size_t texIndex, size_t /*frameIdx*/) const { return _logicalTextures[texIndex].pxFrameWidth; }
	float GetFrameHeight(size_t texIndex, size_t /*frameIdx*/) const { return _logicalTextures[texIndex].pxFrameHeight; }
	vec2d GetFrameSize(size_t texIndex) const { return vec2d{GetFrameWidth(texIndex, 0), GetFrameHeight(texIndex, 0)}; }
	float GetBorderSize(size_t texIndex) const { return _logicalTextures[texIndex].pxBorderSize; }
	unsigned int GetFrameCount(size_t texIndex) const { return static_cast<unsigned int>(_logicalTextures[texIndex].uvFrames.size()); }

	void GetTextureNames(std::vector<std::string> &names, const char *prefix) const;

	float GetCharHeight(size_t fontTexture) const;
	float GetCharWidth(size_t fontTexture) const;

private:
	std::map<std::string, size_t, std::less<>> _mapName_to_Index;// index in _logicalTextures
	std::vector<LogicalTexture> _logicalTextures;

	void CreateChecker(); // Create checker texture without name and with index=0

	using LoadedImages = std::map<std::string, class TgaImage, std::less<>>;
	void CreateAtlas(const LoadedImages &loadedImages, std::vector<PackageSpriteDesc> packageSpriteDescs, bool magFilter);
};
