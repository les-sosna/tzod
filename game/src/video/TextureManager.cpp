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

static FRECT MakeInnerFrameUV(FRECT texOuterFrame, vec2d texFrameBorder, vec2d texSize)
{
	vec2d uvBorderSize = texFrameBorder / texSize;
	return FRECT
	{
		texOuterFrame.left / texSize.x + uvBorderSize.x,
		texOuterFrame.top / texSize.y + uvBorderSize.y,
		texOuterFrame.right / texSize.x - uvBorderSize.x,
		texOuterFrame.bottom / texSize.y - uvBorderSize.y
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
	vec2d uvBorderSize = vec2d{ sd.border, sd.border } / pxTextureSize;
	vec2d uvInnerFrameOffset = sd.atlasOffset / pxTextureSize + uvBorderSize;
	vec2d uvInnerFrameSize = uvFrameSizeWithBorder - uvBorderSize * 2;
	lt.frames.reserve(sd.xframes * sd.yframes);
	for (int y = 0; y < sd.yframes; ++y)
	{
		for (int x = 0; x < sd.xframes; ++x)
		{
			lt.frames.push_back({
				MakeRectWH(uvInnerFrameOffset + uvFrameSizeWithBorder * vec2d{ (float)x, (float)y }, uvInnerFrameSize),
				MakeRectWH(sd.atlasOffset + pxFrameSizeWithBorder * vec2d{ (float)x, (float)y }, pxFrameSizeWithBorder) });
		}
	}

	return lt;
}

void TextureManager::LoadPackage(IRender& render, FS::FileSystem& fs, const std::vector<SpriteDefinition> &definitions)
{
	using ImageMapType = std::map<std::string, TgaImage, std::less<>>;

	std::vector<LogicalTexture> spriteDescs;
	ImageMapType loadedImages;
	struct Frame
	{
		RectRB texBounds;
		size_t frameIndex;
		size_t spriteIndex;
		ImageMapType::iterator sourceImageIt;
	};
	std::vector<Frame> frames;

	// load all images and collect atlas data
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
			frames.push_back({ FRectToRect(lt.frames[frameIndex].texOuterFrame), frameIndex, spriteIndex, imageIt });
		}
	}

	std::stable_sort(frames.begin(), frames.end(), [](const Frame & left, const Frame & right)
	{
		// for same height take narrow first, otherwise tall first
		return (HEIGHT(left.texBounds) == HEIGHT(right.texBounds))
			? (WIDTH(left.texBounds) < WIDTH(right.texBounds))
			: (HEIGHT(left.texBounds) > HEIGHT(right.texBounds));
	});

	EditableImage atlasImage(1536, 1536);

	AtlasPacker packer;
	packer.ExtendCanvas(atlasImage.GetWidth(), atlasImage.GetHeight());

	auto texSize = vec2d{ (float)atlasImage.GetWidth(), (float)atlasImage.GetHeight() };

	for (auto& frame : frames)
	{
		// place frame in the atlas
		RectRB dstRect;
		bool success = packer.PlaceRect(WIDTH(frame.texBounds), HEIGHT(frame.texBounds), dstRect);
		assert(success);

		// blit pixels from the source image
		atlasImage.Blit(dstRect, frame.texBounds.left, frame.texBounds.top, frame.sourceImageIt->second);

		// adjust frame
		auto texOuterFrame = RectToFRect(dstRect);
		float border = definitions[frame.spriteIndex].border;
		spriteDescs[frame.spriteIndex].frames[frame.frameIndex] =
		{
			MakeInnerFrameUV(RectToFRect(dstRect), vec2d{border, border}, texSize),
			texOuterFrame
		};
	}

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

void TextureManager::GetTextureNames(std::vector<std::string> &names,
                                     const char *prefix) const
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
