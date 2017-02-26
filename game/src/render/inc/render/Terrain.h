#pragma once

#include <stddef.h>

class TextureManager;
class RenderContext;

namespace math
{
	struct FRECT;
}

class Terrain
{
public:
	Terrain(TextureManager &tm);
	void Draw(RenderContext &rc, const math::FRECT &bounds, bool drawGrid) const;

private:
	size_t _texBack;
	size_t _texGrid;
};
