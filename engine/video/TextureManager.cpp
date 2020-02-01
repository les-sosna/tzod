#include "inc/video/TextureManager.h"
#include "inc/video/RenderBase.h"
#include "inc/video/TgaImage.h"

#include <fs/FileSystem.h>

#include <cstring>
#include <sstream>

///////////////////////////////////////////////////////////////////////////////

ImageCache::ImageCache()
{
}

ImageCache::~ImageCache()
{
}

ImageView ImageCache::GetImage(FS::FileSystem& fs, std::string_view filePath)
{
	auto imageIt = _loadedImages.find(filePath);
	if (imageIt == _loadedImages.end())
	{
		auto file = fs.Open(filePath)->QueryMap();
		imageIt = _loadedImages.emplace(filePath, TgaImage(file->GetData(), file->GetSize())).first;
	}
	return imageIt->second.GetData();
}

///////////////////////////////////////////////////////////////////////////////

TextureManager::TextureManager()
{
	LogicalTexture tex = {};
	tex.pxFrameWidth = 1;
	tex.pxFrameHeight = 1;
	tex.pxBorderSize = 0;
	tex.frameCount = 1;
	tex.magFilter = false;
	tex.wrappable = false;

	_logicalTextures.push_back(tex);
	_spriteSources.emplace_back();
}

TextureManager::~TextureManager()
{
}

void TextureManager::UnloadAllTextures() noexcept
{
	_mapName_to_Index.clear();
	_logicalTextures.clear();
	_version++;
}

static vec2d GetFrameSizeWithBorder(const PackageSpriteDesc& psd, vec2d pxTextureSize)
{
	vec2d pxAtlasSizeWithBorder = { psd.hasSizeX ? psd.atlasSize.x : pxTextureSize.x - psd.atlasOffset.x,
	                                psd.hasSizeY ? psd.atlasSize.y : pxTextureSize.y - psd.atlasOffset.y };
	return pxAtlasSizeWithBorder / vec2d{ (float)psd.xframes, (float)psd.yframes };
}

static LogicalTexture LogicalTextureFromSpriteDefinition(const PackageSpriteDesc& psd, vec2d pxFrameSizeWithBorder)
{
	LogicalTexture lt;

	// render size
	lt.pxPivot = vec2d{ psd.hasPivotX ? psd.pivot.x : pxFrameSizeWithBorder.x / 2, psd.hasPivotY ? psd.pivot.y : pxFrameSizeWithBorder.y / 2 } * psd.scale;
	lt.pxFrameWidth = pxFrameSizeWithBorder.x * psd.scale.x;
	lt.pxFrameHeight = pxFrameSizeWithBorder.y * psd.scale.y;
	lt.pxBorderSize = psd.border;

	// font
	lt.leadChar = psd.leadChar;

	// frames
	lt.frameCount = psd.xframes * psd.yframes;
	lt.magFilter = psd.magFilter;
	lt.wrappable = psd.wrappable;

	return lt;
}

void TextureManager::LoadPackage(FS::FileSystem& fs, ImageCache &imageCache, const std::vector<PackageSpriteDesc>& packageSpriteDescs)
{
	for (auto& psd : packageSpriteDescs)
	{
		auto image = imageCache.GetImage(fs, psd.textureFilePath);
		auto pxTextureSize = vec2d{ (float)image.width, (float)image.height };
		vec2d pxFrameSizeWithBorder = GetFrameSizeWithBorder(psd, pxTextureSize);

		size_t spriteId = _logicalTextures.size();
		auto emplaced = _mapName_to_Index.emplace(psd.spriteName, spriteId);

		if (emplaced.second)
		{
			_logicalTextures.emplace_back();
			_spriteSources.emplace_back();
		}
		else
		{
			spriteId = emplaced.first->second;
		}

		_logicalTextures[spriteId] = LogicalTextureFromSpriteDefinition(psd, pxFrameSizeWithBorder);

		SpriteSource ss;
		ss.textureFilePath = psd.textureFilePath;
		ss.srcX = static_cast<int>(psd.atlasOffset.x);
		ss.srcY = static_cast<int>(psd.atlasOffset.y);
		ss.pxFrameWidth = static_cast<int>(pxFrameSizeWithBorder.x);
		ss.pxFrameHeight = static_cast<int>(pxFrameSizeWithBorder.y);
		ss.xframes = psd.xframes;
		ss.yframes = psd.yframes;
		_spriteSources[spriteId] = std::move(ss);
	}
	_version++;
}

size_t TextureManager::FindSprite(std::string_view name) const
{
	auto it = _mapName_to_Index.find(name);
	return _mapName_to_Index.end() != it ? it->second : 0;
}

void TextureManager::GetTextureNames(std::vector<std::string> &names, const char *prefix) const
{
	size_t trimLength = prefix ? std::strlen(prefix) : 0;

	names.clear();
	std::map<std::string, size_t>::const_iterator it = _mapName_to_Index.begin();
	for(; it != _mapName_to_Index.end(); ++it )
	{
		if( prefix && 0 != it->first.find(prefix) )
			continue;
		names.push_back(it->first.substr(trimLength));
	}
}

float TextureManager::GetCharHeight(size_t fontTexture) const
{
	return GetSpriteInfo(fontTexture).pxFrameHeight;
}

float TextureManager::GetCharWidth(size_t fontTexture) const
{
	return GetSpriteInfo(fontTexture).pxFrameWidth - 1;
}

static const unsigned char s_blankBytes[] = { 255,255,255,255 };

ImageView TextureManager::GetSpritePixels(FS::FileSystem& fs, ImageCache& imageCache, size_t texIndex, int frameIdx) const
{
	if (texIndex == 0)
	{
		assert(frameIdx == 0);
		ImageView blank = {};
		blank.pixels = s_blankBytes;
		blank.width = 1;
		blank.height = 1;
		blank.stride = 4;
		blank.bpp = 32;
		return blank;
	}
	else
	{
		auto& ss = _spriteSources[texIndex];
		RectRB sourceFrameRect;
		sourceFrameRect.left = ss.srcX + ss.pxFrameWidth * (frameIdx % ss.xframes);
		sourceFrameRect.top = ss.srcY + ss.pxFrameHeight * (frameIdx / ss.xframes);
		sourceFrameRect.right = sourceFrameRect.left + ss.pxFrameWidth;
		sourceFrameRect.bottom = sourceFrameRect.top + ss.pxFrameHeight;
		return imageCache.GetImage(fs, ss.textureFilePath).Slice(sourceFrameRect);
	}
}

size_t TextureManager::GetNextSprite(size_t spriteId) const
{
	size_t nextSpriteId = spriteId + 1;
	return nextSpriteId == _logicalTextures.size() ? 0 : nextSpriteId;
}
