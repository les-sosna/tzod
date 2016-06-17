// TextureManager.h

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
	vec2d uvPivot;

	float pxFrameWidth;
	float pxFrameHeight;
	float pxBorderSize;

	std::vector<FRECT> uvFrames;
};

class TextureManager
{
public:
	TextureManager(TextureManager&&) = default;
	TextureManager(IRender &render);
	~TextureManager();

	IRender& GetRender() const { return _render; }

	int LoadPackage(std::vector<std::tuple<std::shared_ptr<Image>, std::string, LogicalTexture>> definitions);
	void UnloadAllTextures();

	size_t FindSprite(const std::string &name) const;
	const DEV_TEXTURE& GetDeviceTexture(size_t texIndex) const { return _logicalTextures[texIndex].second->id; }
	const LogicalTexture& GetSpriteInfo(size_t texIndex) const { return _logicalTextures[texIndex].first; }
	float GetFrameWidth(size_t texIndex, size_t /*frameIdx*/) const { return _logicalTextures[texIndex].first.pxFrameWidth; }
	float GetFrameHeight(size_t texIndex, size_t /*frameIdx*/) const { return _logicalTextures[texIndex].first.pxFrameHeight; }
	float GetBorderSize(size_t texIndex) const { return _logicalTextures[texIndex].first.pxBorderSize; }
	unsigned int GetFrameCount(size_t texIndex) const { return static_cast<unsigned int>(_logicalTextures[texIndex].first.uvFrames.size()); }

	void GetTextureNames(std::vector<std::string> &names, const char *prefix) const;

	float GetCharHeight(size_t fontTexture) const;

protected:
	IRender &_render;

	struct TexDesc
	{
		DEV_TEXTURE id;
		int width;          // The Width Of The Entire Image.
		int height;         // The Height Of The Entire Image.
		int refCount;       // number of logical textures
	};

	std::list<TexDesc> _devTextures;
	std::map<std::shared_ptr<Image>, std::list<TexDesc>::iterator> _mapImage_to_TexDescIter;
	std::map<std::string, size_t> _mapName_to_Index;// index in _logicalTextures
	std::vector<std::pair<LogicalTexture, std::list<TexDesc>::iterator>> _logicalTextures;

	std::list<TexDesc>::iterator LoadTexture(const std::shared_ptr<Image> &image);

	void CreateChecker(); // Create checker texture without name and with index=0
};

std::vector<std::tuple<std::shared_ptr<Image>, std::string, LogicalTexture>>
ParsePackage(const std::string &packageName, std::shared_ptr<FS::MemMap> file, FS::FileSystem &fs);

std::vector<std::tuple<std::shared_ptr<Image>, std::string, LogicalTexture>>
ParseDirectory(const std::string &dirName, const std::string &texPrefix, FS::FileSystem &fs);
