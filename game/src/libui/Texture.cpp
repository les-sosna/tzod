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
