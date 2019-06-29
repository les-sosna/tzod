#include "inc/video/TextureManager.h"
#include "inc/video/RenderBase.h"
#include "inc/video/TgaImage.h"
#include "inc/video/EditableImage.h"
#include "AtlasPacker.h"

#define TRACE(...)

#include <fs/FileSystem.h>

#include <cstring>
#include <sstream>
#include <stdexcept>

///////////////////////////////////////////////////////////////////////////////

class CheckerImage final
	: public Image
{
public:
	// Image
	const void* GetData() const override { return _bytes; }
	unsigned int GetBpp() const override { return 24; }
	unsigned int GetWidth() const override { return 4; }
	unsigned int GetHeight() const override { return 4; }

private:
	static const unsigned char _bytes[];
};

const unsigned char CheckerImage::_bytes[] = {
	0,  0,  0,     0,  0,  0,   255,255,255,   255,255,255,
	0,  0,  0,     0,  0,  0,   255,255,255,   255,255,255,
	255,255,255,   255,255,255,     0,  0,  0,     0,  0,  0,
	255,255,255,   255,255,255,     0,  0,  0,     0,  0,  0,
};


///////////////////////////////////////////////////////////////////////////////

TextureManager::TextureManager(IRender &render)
	: _RenderHack(render)
{
	CreateChecker(render);
}

TextureManager::~TextureManager()
{
	assert(_devTextures.empty());
}

void TextureManager::UnloadAllTextures(IRender& render) noexcept
{
	for (auto &t: _devTextures)
		render.TexFree(t.id);
	_devTextures.clear();
	_mapName_to_Index.clear();
	_logicalTextures.clear();
}

static vec2d GetImageSize(const Image& image)
{
	return vec2d{ (float)image.GetWidth(), (float)image.GetHeight() };
}

void TextureManager::CreateChecker(IRender& render)
{
	assert(_logicalTextures.empty()); // to be sure that checker will get index 0
	assert(_mapName_to_Index.empty());
	TRACE("Creating checker texture...");

	TexDesc td = {};
	CheckerImage c;
	if( !render.TexCreate(td.id, c, false) )
	{
		TRACE("ERROR: error in render device");
		assert(false);
		return;
	}

	_devTextures.push_front(td);
	auto texDescIter = _devTextures.begin();
	texDescIter->refCount++;

	LogicalTexture tex;
	tex.pxPivot = GetImageSize(c) * 4;
	tex.pxFrameWidth = GetImageSize(c).x * 8;
	tex.pxFrameHeight = GetImageSize(c).y * 8;
	tex.pxBorderSize = 0;
	tex.uvFrames = { { 0,0,2,2 } };

	_logicalTextures.emplace_back(tex, texDescIter);
}

static FRECT MakeInnerFrameUV(RectRB texOuterFrame, vec2d texFrameBorder, vec2d texSize)
{
	vec2d uvBorderSize = texFrameBorder / texSize;
	return FRECT
	{
		(float)texOuterFrame.left / texSize.x + uvBorderSize.x,
		(float)texOuterFrame.top / texSize.y + uvBorderSize.y,
		(float)texOuterFrame.right / texSize.x - uvBorderSize.x,
		(float)texOuterFrame.bottom / texSize.y - uvBorderSize.y
	};
}

static LogicalTexture LogicalTextureFromSpriteDefinition(const PackageSpriteDesc& sd, vec2d pxTextureSize)
{
	LogicalTexture lt;

	vec2d pxAtlasSizeWithBorder = { sd.hasSizeX ? sd.atlasSize.x : pxTextureSize.x - sd.atlasOffset.x, sd.hasSizeY ? sd.atlasSize.y : pxTextureSize.y - sd.atlasOffset.y };
	vec2d pxFrameSizeWithBorder = pxAtlasSizeWithBorder / vec2d{ (float)sd.xframes, (float)sd.yframes };
	vec2d uvFrameSizeWithBorder = pxFrameSizeWithBorder / pxTextureSize;

	// render size
	lt.pxPivot = vec2d{ sd.hasPivotX ? sd.pivot.x : pxFrameSizeWithBorder.x / 2, sd.hasPivotY ? sd.pivot.y : pxFrameSizeWithBorder.y / 2 } * sd.scale;
	lt.pxFrameWidth = pxTextureSize.x * sd.scale.x * uvFrameSizeWithBorder.x;
	lt.pxFrameHeight = pxTextureSize.y * sd.scale.y * uvFrameSizeWithBorder.y;
	lt.pxBorderSize = sd.border;

	// font
	lt.leadChar = sd.leadChar;

	// frames
	vec2d texBorderSize = vec2d{ sd.border, sd.border };
	lt.uvFrames.reserve(sd.xframes * sd.yframes);
	for (int y = 0; y < sd.yframes; ++y)
	{
		for (int x = 0; x < sd.xframes; ++x)
		{
			auto texOuterFrame = FRectToRect(MakeRectWH(sd.atlasOffset + pxFrameSizeWithBorder * vec2d{ (float)x, (float)y }, pxFrameSizeWithBorder));
			lt.uvFrames.push_back(MakeInnerFrameUV(texOuterFrame, texBorderSize, pxTextureSize));
		}
	}

	return lt;
}

void TextureManager::LoadPackage(IRender& render, FS::FileSystem& fs, const std::vector<PackageSpriteDesc>& packageSpriteDescs)
{
	LoadedImages loadedImages;

	std::vector<PackageSpriteDesc> magFilterOn;
	std::vector<PackageSpriteDesc> magFilterOff;

	// load all images
	for (auto& item : packageSpriteDescs)
	{
		auto imageIt = loadedImages.find(item.textureFilePath);
		if (imageIt == loadedImages.end())
		{
			auto file = fs.Open(item.textureFilePath)->QueryMap();
			imageIt = loadedImages.emplace(item.textureFilePath, TgaImage(file->GetData(), file->GetSize())).first;
		}

		if (item.wrappable)
		{
			CreateAtlas(render, fs, loadedImages, std::vector<PackageSpriteDesc>(1, item), item.magFilter);
		}
		else
		{
			if (item.magFilter)
				magFilterOn.push_back(item);
			else
				magFilterOff.push_back(item);
		}
	}

	CreateAtlas(render, fs, loadedImages, std::move(magFilterOn), true);
	CreateAtlas(render, fs, loadedImages, std::move(magFilterOff), false);

	// unload unused textures
	for (auto it = _devTextures.begin(); _devTextures.end() != it; )
	{
		if (0 == it->refCount)
		{
			render.TexFree(it->id);
			it = _devTextures.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void TextureManager::CreateAtlas(IRender& render, FS::FileSystem& fs, const LoadedImages& loadedImages, std::vector<PackageSpriteDesc> packageSpriteDescs, bool magFilter)
{
	if (packageSpriteDescs.empty())
		return;

	int gutters = magFilter ? 1 : 0;

	int totalFrames = 0;
	for (auto& psd : packageSpriteDescs)
		totalFrames += psd.xframes* psd.yframes;

	struct AtlasFrame
	{
		int width;
		int height;
		int srcX;
		int srcY;
		int dstX;
		int dstY;
	};
	std::vector<AtlasFrame> atlasFrames;
	atlasFrames.reserve(totalFrames);

	int totalTexels = 0;
	int minAtlasWidth = 0;

	for (auto& psd : packageSpriteDescs)
	{
		auto imageIt = loadedImages.find(psd.textureFilePath);
		vec2d pxTextureSize = GetImageSize(imageIt->second);
		vec2d pxAtlasSizeWithBorder = { psd.hasSizeX ? psd.atlasSize.x : pxTextureSize.x - psd.atlasOffset.x,
		                                psd.hasSizeY ? psd.atlasSize.y : pxTextureSize.y - psd.atlasOffset.y };
		vec2d pxFrameSizeWithBorder = pxAtlasSizeWithBorder / vec2d{ (float)psd.xframes, (float)psd.yframes };

		for (int y = 0; y < psd.yframes; ++y)
		{
			for (int x = 0; x < psd.xframes; ++x)
			{
				auto src = psd.atlasOffset + pxFrameSizeWithBorder * vec2d{ (float)x, (float)y };
				int widthWithGutters = (int)pxFrameSizeWithBorder.x + gutters * 2;
				int heightWithGutters = (int)pxFrameSizeWithBorder.y + gutters * 2;
				atlasFrames.push_back({ widthWithGutters, heightWithGutters, (int)src.x, (int)src.y });
				totalTexels += widthWithGutters * heightWithGutters;
				minAtlasWidth = std::max(minAtlasWidth, widthWithGutters);
			}
		}
	}

	std::vector<int> sortedFrames(atlasFrames.size());
	for (int i = 0; i < sortedFrames.size(); i++)
		sortedFrames[i] = i;

	std::stable_sort(sortedFrames.begin(), sortedFrames.end(),
		[&](int leftIndex, int rightIndex)
		{
			const AtlasFrame& left = atlasFrames[leftIndex];
			const AtlasFrame& right = atlasFrames[rightIndex];
			// for same height take narrow first, otherwise tall first
			return (left.height == right.height) ? (left.width < right.width) : (left.height > right.height);
		});

	double idealSquareSide = std::sqrt(totalTexels);
	double nextMultiple64 = std::ceil(idealSquareSide / 64) * 64;
	int atlasWidth = std::max(minAtlasWidth, (int)nextMultiple64);

	AtlasPacker packer;
	packer.ExtendCanvas(atlasWidth, atlasWidth * 100); // unlimited height
	for (auto index : sortedFrames)
	{
		auto& atlasFrame = atlasFrames[index];
		bool success = packer.PlaceRect(atlasFrame.width, atlasFrame.height, atlasFrame.dstX, atlasFrame.dstY);
		assert(success);
	}

	// now when we know the atlas height we can blit pixels from the source images
	EditableImage atlasImage(atlasWidth, packer.GetContentHeight()); // actual height
	auto atlasSize = GetImageSize(atlasImage);

	TexDesc &td = _devTextures.emplace_front();
	auto devTexIt = _devTextures.begin();

	LoadedImages::const_iterator spriteSourceImageIt;
	LogicalTexture* currentLT = nullptr;
	int spriteIndex = 0;
	int frameIndex = 0;

	for (auto& atlasFrame : atlasFrames)
	{
		auto texOuterFrameNoGutters = RectRB
		{
			atlasFrame.dstX + gutters,
			atlasFrame.dstY + gutters,
			atlasFrame.dstX + atlasFrame.width - gutters,
			atlasFrame.dstY + atlasFrame.height - gutters
		};

		auto& psd = packageSpriteDescs[spriteIndex];
		auto numFrames = psd.xframes * psd.yframes;
		if (frameIndex == 0)
		{
			spriteSourceImageIt = loadedImages.find(psd.textureFilePath);

			auto emplaced = _mapName_to_Index.emplace(psd.spriteName, _logicalTextures.size());
			if (emplaced.second)
			{
				// define new texture
				currentLT = &_logicalTextures.emplace_back(LogicalTextureFromSpriteDefinition(psd, GetImageSize(spriteSourceImageIt->second)), devTexIt).first;
			}
			else
			{
				// replace existing logical texture
				auto& existing = _logicalTextures[emplaced.first->second];
				assert(existing.second->refCount > 0);
				existing.first = LogicalTextureFromSpriteDefinition(psd, GetImageSize(spriteSourceImageIt->second));
				existing.second->refCount--;
				existing.second = devTexIt;
				currentLT = &existing.first;
			}
#if 0
			// export sprites
			EditableImage exportedImage(WIDTH(texOuterFrameNoGutters) * psd.xframes, HEIGHT(texOuterFrameNoGutters) * psd.yframes);
			for (int y = 0; y < psd.yframes; y++)
			{
				for (int x = 0; x < psd.xframes; x++)
				{
					RectRB dstRect = { WIDTH(texOuterFrameNoGutters) * x, HEIGHT(texOuterFrameNoGutters) * y,
						WIDTH(texOuterFrameNoGutters) * (x + 1), HEIGHT(texOuterFrameNoGutters) * (y + 1) };
					exportedImage.Blit(dstRect, 0, (x + y * psd.xframes)[&atlasFrame].srcX, (x + y * psd.xframes)[&atlasFrame].srcY, spriteSourceImageIt->second);
				}
			}
			std::vector<uint8_t> buffer(GetTgaByteSize(exportedImage));
			WriteTga(exportedImage, buffer.data(), buffer.size());
			std::string exportedPath = std::string("export/") + psd.spriteName + ".tga";

			auto pd = exportedPath.rfind('/');
			auto exportedDirName = exportedPath.substr(0, pd);
			auto exportedFileName = exportedPath.substr(pd + 1);
			auto dir = fs.GetFileSystem(exportedDirName, true /*create*/);
			dir->Open(exportedFileName, FS::FileMode::ModeWrite)->QueryStream()->Write(buffer.data(), buffer.size());

			std::ostringstream metadata;
			if (psd.border != 0)
				metadata << "border = " << psd.border << std::endl;
			if (psd.hasPivotX) // todo: check not center
				metadata << "xpivot = " << psd.pivot.x << std::endl;
			if (psd.hasPivotY)
				metadata << "ypivot = " << psd.pivot.y << std::endl;
			if (psd.xframes != 1)
				metadata << "xframes = " << psd.xframes << std::endl;
			if (psd.yframes != 1)
				metadata << "yframes = " << psd.yframes << std::endl;
			if (psd.scale.x != 1)
				metadata << "xscale = " << psd.scale.x << std::endl;
			if (psd.scale.y != 1)
				metadata << "yscale = " << psd.scale.y << std::endl;
			if (psd.magFilter)
				metadata << "magfilter = true" << std::endl;
			if (psd.wrappable)
				metadata << "wrappable = true" << std::endl;
			if (psd.leadChar != ' ')
				metadata << "leadchar = '" << (char)psd.leadChar << "'" << std::endl;
			auto strbuf = metadata.str();
			if (!strbuf.empty())
			{
				auto exportedFileNameNoExt = exportedFileName;
				exportedFileNameNoExt.erase(exportedFileNameNoExt.size() - 4);
				dir->Open(exportedFileNameNoExt + ".lua", FS::FileMode::ModeWrite)->QueryStream()->Write(strbuf.data(), strbuf.size());
			}
#endif
		}
		assert(loadedImages.end() != spriteSourceImageIt);

		atlasImage.Blit(texOuterFrameNoGutters, gutters, atlasFrame.srcX, atlasFrame.srcY, spriteSourceImageIt->second);

		// replace uv frame
		currentLT->uvFrames[frameIndex] = MakeInnerFrameUV(texOuterFrameNoGutters, vec2d{ psd.border, psd.border }, atlasSize);

		++frameIndex;
		if (frameIndex == numFrames)
		{
			frameIndex = 0;
			++spriteIndex;
		}
	}

	// allocate hardware texture for atlas
	if (!render.TexCreate(td.id, atlasImage, magFilter))
		throw std::runtime_error("error in render device");

	td.refCount = static_cast<int>(packageSpriteDescs.size());
}

size_t TextureManager::FindSprite(std::string_view name) const
{
	std::map<std::string, size_t>::const_iterator it = _mapName_to_Index.find(name);
	if( _mapName_to_Index.end() != it )
		return it->second;

	return 0; // index of checker texture
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
