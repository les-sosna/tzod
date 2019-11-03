#pragma once
#include "RenderBase.h"
#include <list>
#include <vector>

class TextureManager;
class ImageCache;

namespace FS
{
	class FileSystem;
}

struct RenderBindingEnv
{
	FS::FileSystem& fs;
	const TextureManager& texman;
	ImageCache& imageCache;
	IRender& render;
};

class RenderBinding
{
public:
	explicit RenderBinding(IRender& render);
	~RenderBinding();

	void Update(const RenderBindingEnv &env);

	void UnloadAllTextures(IRender& render) noexcept;

	const DEV_TEXTURE& GetDeviceTexture(size_t texIndex) const noexcept
	{
		return _sprites[texIndex].descIt->id;
	}

	const FRECT* GetUVFrames(size_t texIndex) const noexcept
	{
		const SpriteRef& spriteRef = _sprites[texIndex];
		return &spriteRef.descIt->uvFrames[spriteRef.firstFrameIndex];
	}


private:
	struct TexDesc
	{
		DEV_TEXTURE id;
		std::vector<FRECT> uvFrames;
	};

	struct SpriteRef
	{
		std::list<TexDesc>::iterator descIt;
		int firstFrameIndex;
	};

	std::vector<SpriteRef> _sprites;
	std::list<TexDesc> _devTextures;

	SpriteRef& EnsureSpriteRef(size_t spriteId);
	void CreateAtlas(const RenderBindingEnv& env, std::vector<size_t> sprites, bool magFilter, int gutters);
};

