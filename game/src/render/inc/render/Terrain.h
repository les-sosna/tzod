#pragma once

#include <stddef.h>

class TextureManager;
class DrawingContext;

class Terrain
{
public:
	Terrain(TextureManager &tm);
	void Draw(DrawingContext &dc, float sizeX, float sizeY, bool drawGrid) const;

private:
	size_t _texBack;
	size_t _texGrid;
};
