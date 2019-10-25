#include "inc/video/RenderBinding.h"

RenderBinding::RenderBinding(IRender& render)
{
}

RenderBinding::~RenderBinding()
{
	assert(_devTextures.empty());
}

void RenderBinding::Update(const TextureManager& tm)
{

}

void RenderBinding::UnloadAllTextures(IRender& render) noexcept
{
	for (auto& t : _devTextures)
		render.TexFree(t.id);
	_devTextures.clear();
	_logicalTexturesMapping.clear();
}

