#pragma once
#include "RenderBase.h"
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
	explicit TextureManager(IRender &render);
	~TextureManager();

	IRender& _RenderHack;

	void LoadPackage(IRender& render, FS::FileSystem& fs, const std::vector<PackageSpriteDesc>& packageSpriteDescs);
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
		int refCount;       // number of logical textures
	};

	std::list<TexDesc> _devTextures;
	std::map<std::string, size_t, std::less<>> _mapName_to_Index;// index in _logicalTextures
	std::vector<std::pair<LogicalTexture, std::list<TexDesc>::iterator>> _logicalTextures;

	void CreateChecker(IRender& render); // Create checker texture without name and with index=0

	using LoadedImages = std::map<std::string, class TgaImage, std::less<>>;
	void CreateAtlas(IRender& render, FS::FileSystem& fs, const LoadedImages &loadedImages, std::vector<PackageSpriteDesc> packageSpriteDescs, bool magFilter);
};
