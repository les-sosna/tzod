#include "inc/video/RenderBinding.h"
#include "inc/video/TextureManager.h"

RenderBinding::RenderBinding(IRender& render)
{
}

RenderBinding::~RenderBinding()
{
	assert(_devTextures.empty());
}

void RenderBinding::Update(const TextureManager& tm, IRender& render)
{


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

void RenderBinding::UnloadAllTextures(IRender& render) noexcept
{
	for (auto& t : _devTextures)
		render.TexFree(t.id);
	_devTextures.clear();
	_sprites.clear();
}

