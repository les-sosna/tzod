#include "inc/video/RenderBinding.h"
#include "inc/video/EditableImage.h"
#include "inc/video/TextureManager.h"
#include "AtlasPacker.h"
#include <stdexcept>

RenderBinding::RenderBinding(IRender& render)
{
}

RenderBinding::~RenderBinding()
{
	assert(_devTextures.empty());
}

void RenderBinding::Update(const RenderBindingEnv& env)
{
	std::vector<size_t> magFilterOn;
	std::vector<size_t> magFilterOff;

	size_t spriteId = 0;
	do
	{
		auto spriteInfo = env.texman.GetSpriteInfo(spriteId);
		if (spriteInfo.wrappable)
		{
			CreateAtlas(env, std::vector<size_t>(1, spriteId), spriteInfo.magFilter, 0 /*gutters*/);
		}
		else
		{
			if (spriteInfo.magFilter)
				magFilterOn.push_back(spriteId);
			else
				magFilterOff.push_back(spriteId);
		}
		spriteId = env.texman.GetNextSprite(spriteId);
	} while (spriteId != 0);

	CreateAtlas(env, std::move(magFilterOn), true, 1);
	CreateAtlas(env, std::move(magFilterOff), false, 1);
}

void RenderBinding::UnloadAllTextures(IRender& render) noexcept
{
	for (auto& t : _devTextures)
		render.TexFree(t.id);
	_devTextures.clear();
	_sprites.clear();
}

RenderBinding::SpriteRef& RenderBinding::EnsureSpriteRef(size_t spriteId)
{
	if (spriteId + 1 > _sprites.size())
	{
		_sprites.resize(spriteId + 1);
	}
	return _sprites[spriteId];
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

void RenderBinding::CreateAtlas(const RenderBindingEnv& env, std::vector<size_t> sprites, bool magFilter, int gutters)
{
	if (sprites.empty())
		return;

	int totalFrames = 0;
	for (auto spriteId : sprites)
		totalFrames += env.texman.GetFrameCount(spriteId);

	struct AtlasEntry
	{
		ImageView source;
		int dstX;
		int dstY;
	};
	std::vector<AtlasEntry> atlasEntries;
	atlasEntries.reserve(totalFrames);

	int totalTexels = 0;
	int minAtlasWidth = 0;

	TexDesc& td = _devTextures.emplace_front();
	td.uvFrames.resize(totalFrames);

	int currentFrame = 0;
	for (auto spriteId : sprites)
	{
		auto& spriteRef = EnsureSpriteRef(spriteId);
		spriteRef.descIt = _devTextures.begin();
		spriteRef.firstFrameIndex = currentFrame;

		for (size_t frame = 0; frame < env.texman.GetFrameCount(spriteId); frame++)
		{
			AtlasEntry entry = { env.texman.GetSpritePixels(env.fs, env.imageCache, spriteId, frame) };
			atlasEntries.push_back(entry);
			int widthWithGutters = entry.source.width + gutters * 2;
			int heightWithGutters = entry.source.height + gutters * 2;
			totalTexels += widthWithGutters * heightWithGutters;
			minAtlasWidth = std::max(minAtlasWidth, widthWithGutters);
			currentFrame++;
		}
	}

	std::vector<int> sortedFrames(atlasEntries.size());
	for (int i = 0; i < sortedFrames.size(); i++)
		sortedFrames[i] = i;

	std::stable_sort(sortedFrames.begin(), sortedFrames.end(),
		[&](int leftIndex, int rightIndex)
	{
		const AtlasEntry& left = atlasEntries[leftIndex];
		const AtlasEntry& right = atlasEntries[rightIndex];
		// for same height take narrow first, otherwise tall first
		return (left.source.height == right.source.height) ? (left.source.width < right.source.width) : (left.source.height > right.source.height);
	});

	double idealSquareSide = std::sqrt(totalTexels);
	double nextMultiple64 = std::ceil(idealSquareSide / 64) * 64;
	int atlasWidth = std::max(minAtlasWidth, (int)nextMultiple64);

	AtlasPacker packer;
	packer.ExtendCanvas(atlasWidth, atlasWidth * 100); // unlimited height
	for (auto index : sortedFrames)
	{
		auto& atlasEntry = atlasEntries[index];
		bool success = packer.PlaceRect(atlasEntry.source.width + gutters * 2, atlasEntry.source.height + gutters * 2, atlasEntry.dstX, atlasEntry.dstY);
		assert(success);
	}

	// now when we know the atlas height we can blit pixels from the source images
	EditableImage atlasImage(atlasWidth, packer.GetContentHeight()); // actual height
	vec2d atlasSize = { (float)atlasWidth, (float)packer.GetContentHeight() };
	currentFrame = 0;
	for (auto spriteId : sprites)
	{
		float pxBorder = env.texman.GetBorderSize(spriteId);
		for (size_t frame = 0; frame < env.texman.GetFrameCount(spriteId); frame++)
		{
			auto atlasEntry = atlasEntries[currentFrame];
			RectRB dstOuterFrameNoGutters = {
				atlasEntry.dstX + gutters,
				atlasEntry.dstY + gutters,
				atlasEntry.dstX + atlasEntry.source.width + gutters,
				atlasEntry.dstY + atlasEntry.source.height + gutters
			};
			atlasImage.Blit(dstOuterFrameNoGutters.left, dstOuterFrameNoGutters.top, gutters, atlasEntry.source);
			td.uvFrames[currentFrame] = MakeInnerFrameUV(dstOuterFrameNoGutters, vec2d{ pxBorder, pxBorder }, atlasSize);
			currentFrame++;
		}
	}

	// upload atlas to GPU
	if (!env.render.TexCreate(td.id, atlasImage.GetData(), magFilter))
		throw std::runtime_error("error in render device");
}
