#include "inc/video/TextureManager.h"
#include "inc/video/RenderBase.h"
#include "inc/video/ImageLoader.h"
#include "inc/video/EditableImage.h"
#include "AtlasPacker.h"

#define TRACE(...)

#include <fs/FileSystem.h>

#include <cstring>
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

void TextureManager::CreateChecker(IRender& render)
{
	assert(_logicalTextures.empty()); // to be sure that checker will get index 0
	assert(_mapName_to_Index.empty());
	TRACE("Creating checker texture...");

	TexDesc td;
	CheckerImage c;
	if( !render.TexCreate(td.id, c, false) )
	{
		TRACE("ERROR: error in render device");
		assert(false);
		return;
	}
	td.width = c.GetWidth();
	td.height = c.GetHeight();
	td.refCount = 0;

	_devTextures.push_front(td);

	auto texDescIter = _devTextures.begin();
	texDescIter->refCount++;

	LogicalTexture tex;
	tex.pxPivot = vec2d{ (float)td.width, (float)td.height } * 4;
	tex.pxFrameWidth = (float) td.width * 8;
	tex.pxFrameHeight = (float) td.height * 8;
	tex.pxBorderSize = 0;
	tex.frames = { {{ 0,0,2,2 }, {}} };

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

static LogicalTexture LogicalTextureFromSpriteDefinition(const SpriteDefinition& sd, vec2d pxTextureSize)
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
	lt.frames.reserve(sd.xframes * sd.yframes);
	for (int y = 0; y < sd.yframes; ++y)
	{
		for (int x = 0; x < sd.xframes; ++x)
		{
			auto texOuterFrame = FRectToRect(MakeRectWH(sd.atlasOffset + pxFrameSizeWithBorder * vec2d{ (float)x, (float)y }, pxFrameSizeWithBorder));
			lt.frames.push_back({ MakeInnerFrameUV(texOuterFrame, texBorderSize, pxTextureSize), texOuterFrame });
		}
	}

	return lt;
}

void TextureManager::LoadPackage(IRender& render, FS::FileSystem& fs, const std::vector<SpriteDefinition> &definitions)
{
	using ImageMapType = std::map<std::string, TgaImage, std::less<>>;

	std::vector<LogicalTexture> spriteDescs;
	ImageMapType loadedImages;
	struct AtlasFrame
	{
		ImageMapType::iterator sourceImageIt;
		size_t frameIndex;
		size_t spriteIndex;
		RectRB texPackedWithGutters;
	};
	std::vector<AtlasFrame> atlasFrames;

	constexpr int gutters = 1;
	int totalTexels = 0;

	// load all images and collect remaining atlas data
	for (auto& item : definitions)
	{
		auto imageIt = loadedImages.find(item.textureFilePath);
		if (imageIt == loadedImages.end())
		{
			auto file = fs.Open(item.textureFilePath)->QueryMap();
			imageIt = loadedImages.emplace(item.textureFilePath, TgaImage(file->GetData(), file->GetSize())).first;
		}

		LogicalTexture lt = LogicalTextureFromSpriteDefinition(item, vec2d{ (float)imageIt->second.GetWidth(), (float)imageIt->second.GetHeight() });
		size_t spriteIndex = spriteDescs.size();
		spriteDescs.push_back(lt);

		for (size_t frameIndex = 0; frameIndex != lt.frames.size(); ++frameIndex)
		{
			atlasFrames.push_back({ imageIt, frameIndex, spriteIndex });
			auto outerFrame = lt.frames[frameIndex].texOuterFrameSource;
			totalTexels += (WIDTH(outerFrame) + gutters * 2) * (HEIGHT(outerFrame) + gutters * 2);
		}
	}

	std::stable_sort(atlasFrames.begin(), atlasFrames.end(), [&](const AtlasFrame& left, const AtlasFrame& right)
		{
			int leftWidth = WIDTH(spriteDescs[left.spriteIndex].frames[left.frameIndex].texOuterFrameSource);
			int leftHeight = HEIGHT(spriteDescs[left.spriteIndex].frames[left.frameIndex].texOuterFrameSource);
			int rightWidth = WIDTH(spriteDescs[right.spriteIndex].frames[right.frameIndex].texOuterFrameSource);
			int rightHeight = HEIGHT(spriteDescs[right.spriteIndex].frames[right.frameIndex].texOuterFrameSource);

			// for same height take narrow first, otherwise tall first
			return (leftHeight == rightHeight) ? (leftWidth < rightWidth) : (leftHeight > rightHeight);
		});

	double idealSquareSide = std::sqrt(totalTexels);
	double nextMultiple64 = std::ceil(idealSquareSide / 64) * 64;

	int atlasSize = (int)nextMultiple64;

	AtlasPacker packer;
	packer.ExtendCanvas(atlasSize, atlasSize * 100); // unlimited height

	for (auto& atlasFrame : atlasFrames)
	{
		// place frame with gutters in the atlas
		auto texOuterFrame = spriteDescs[atlasFrame.spriteIndex].frames[atlasFrame.frameIndex].texOuterFrameSource;
		int widthWithGutters = WIDTH(texOuterFrame) + gutters * 2;
		int heightWithGutters = HEIGHT(texOuterFrame) + gutters * 2;
		bool success = packer.PlaceRect(widthWithGutters, heightWithGutters, atlasFrame.texPackedWithGutters);
	}

	// blit pixels from the source images and adjust uv frames
	EditableImage atlasImage(atlasSize, packer.GetContentHeight()); // actual height
	auto texSize = vec2d{ (float)atlasImage.GetWidth(), (float)atlasImage.GetHeight() };
	for (auto& atlasFrame : atlasFrames)
	{
		auto& spriteFrame = spriteDescs[atlasFrame.spriteIndex].frames[atlasFrame.frameIndex];
		auto texOuterFramePacked = atlasFrame.texPackedWithGutters;
		texOuterFramePacked.left += gutters;
		texOuterFramePacked.top += gutters;
		texOuterFramePacked.right -= gutters;
		texOuterFramePacked.bottom -= gutters;
		atlasImage.Blit(texOuterFramePacked, gutters, spriteFrame.texOuterFrameSource.left, spriteFrame.texOuterFrameSource.top, atlasFrame.sourceImageIt->second);
		float border = definitions[atlasFrame.spriteIndex].border;
		spriteFrame.uvInnerFrame = MakeInnerFrameUV(texOuterFramePacked, vec2d{ border, border }, texSize);
	}

	// allocate hardware texture for atlas
	TexDesc td;
	if (!render.TexCreate(td.id, atlasImage, false/*FIXME: magFilter*/))
		throw std::runtime_error("error in render device");

	td.width = atlasImage.GetWidth();
	td.height = atlasImage.GetHeight();
	td.refCount = static_cast<int>(spriteDescs.size());

	_devTextures.push_front(td);
	auto devTexIt = _devTextures.begin();

	for (size_t i = 0; i < definitions.size(); ++i)
	{
		auto& item = definitions[i];
		auto& lt = spriteDescs[i];

		auto emplaced = _mapName_to_Index.emplace(item.spriteName, _logicalTextures.size());
		if( emplaced.second )
		{
			// define new texture
			_logicalTextures.emplace_back(lt, devTexIt);
		}
		else
		{
			// replace existing logical texture
			auto &existing = _logicalTextures[emplaced.first->second];
			assert(existing.second->refCount > 0);
			existing.first = lt;
			existing.second->refCount--;
			existing.second = devTexIt;
		}
	}

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
