#pragma once
#include <math/MyMath.h>
#include <string>

class TextureManager;

namespace UI
{
	class Texture
	{
		enum SpecialTextureId : size_t
		{
			TextureIdEmpty = static_cast<size_t>(-1),
			TextureIdOutOfDate = static_cast<size_t>(-2)
		};

	public:
		Texture() = default;

		Texture(std::string textureName)
			: _textureName(std::move(textureName))
			, _cachedTextureId(-2)
		{}

		Texture(const char *textureName)
			: _textureName(textureName ? textureName : std::string())
			, _cachedTextureId(textureName ? TextureIdOutOfDate : TextureIdEmpty)
		{}

		size_t GetTextureId(const TextureManager &texman) const;
		vec2d GetTextureSize(const TextureManager &texman) const;

		bool Empty() const { return TextureIdEmpty == _cachedTextureId; }

	private:
		std::string _textureName;
		mutable size_t _cachedTextureId = TextureIdEmpty;
	};
}
