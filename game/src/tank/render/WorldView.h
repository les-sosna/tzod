#pragma once

#include <cstddef>

class TextureManager;
class World;
struct FRECT;
struct IRender;

class WorldView
{
public:
    WorldView(IRender &render, TextureManager &texman);
	void Render(World &world, const FRECT &view, bool editorMode) const;

private:
    IRender &_render;
    TextureManager &_texman;
	size_t _texBack;
	size_t _texGrid;
	void DrawBackground(float sizeX, float sizeY, size_t tex) const;
};
