#pragma once

#include <stddef.h>

class TextureManager;
class RenderContext;
class World;

namespace math
{
	struct FRECT;
}

class Terrain final
{
public:
	Terrain(TextureManager &tm);
	void Draw(RenderContext &rc, const World& world, bool drawGrid, bool drawBackground) const;

private:
	size_t _texBack;
	size_t _texGrid;
	size_t _texWater;
};
