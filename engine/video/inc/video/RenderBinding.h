#pragma once
#include "RenderBase.h"
#include <list>
#include <vector>

class TextureManager;

class RenderBinding
{
public:
	explicit RenderBinding(IRender& render);
	~RenderBinding();

	void Update(const TextureManager& tm, IRender& render);

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
		unsigned int firstFrameIndex;
	};

	std::vector<SpriteRef> _sprites;
	std::list<TexDesc> _devTextures;
};

