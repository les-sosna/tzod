#include "inc/ui/Texture.h"
#include <video/TextureManager.h>

using namespace UI;

size_t Texture::GetTextureId(const TextureManager &texman) const
{
	if (-2 == _cachedTextureId)
	{
		_cachedTextureId = texman.FindSprite(_textureName);
	}
	return _cachedTextureId;
}

vec2d Texture::GetTextureSize(const TextureManager &texman) const
{
	if (Empty())
	{
		return vec2d{};
	}
	else
	{
		size_t texId = GetTextureId(texman);
		return vec2d{ texman.GetFrameWidth(texId, 0), texman.GetFrameHeight(texId, 0) };
	}
}
