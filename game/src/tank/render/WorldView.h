#pragma once

#include "video/RenderBase.h"
#include <vector>

class TextureManager;
class World;
struct FRECT;

class WorldView
{
public:
    WorldView(IRender &render, TextureManager &texman);
	void Render(World &world, bool editorMode) const;

private:
	void RenderInternal(World &world, const FRECT &view, bool editorMode) const;
    
    IRender &_render;
    TextureManager &_texman;
	size_t _texBack;
	size_t _texGrid;
	void DrawBackground(float sizeX, float sizeY, size_t tex) const;

	mutable std::vector<MyLine> _dbgLineBuffer;
	void DbgLine(const vec2d &v1, const vec2d &v2, SpriteColor color = 0x00ff00ff) const
#ifdef NDEBUG
	{} // do nothing in release mode
#endif
    ;
    
};
