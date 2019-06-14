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

	void Update(const TextureManager& tm);

	void UnloadAllTextures(IRender& render) noexcept;

	const DEV_TEXTURE& GetDeviceTexture(size_t texIndex) const { return _logicalTexturesMapping[texIndex]->id; }


private:
	struct TexDesc
	{
		DEV_TEXTURE id;
		int refCount;       // number of logical textures
	};

	std::vector<std::list<TexDesc>::iterator> _logicalTexturesMapping;
	std::list<TexDesc> _devTextures;
};

