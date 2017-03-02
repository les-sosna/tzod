#pragma once
#include <string>

class TextureManager;

namespace UI
{
	class Texture
	{
	public:
		Texture() = default;

		Texture(std::string textureName)
			: _textureName(std::move(textureName))
			, _cachedTextureId(-2)
		{}

		Texture(const char *textureName)
			: _textureName(textureName ? textureName : std::string())
			, _cachedTextureId(textureName ? -2 : -1)
		{}

		size_t GetTextureId(const TextureManager &texman) const;

		bool Empty() const { return -1 == _cachedTextureId; }

	private:
		std::string _textureName;
		mutable size_t _cachedTextureId = -1;
	};
}
