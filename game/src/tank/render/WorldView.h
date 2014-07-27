#pragma once

#include "Terrain.h"

class DrawingContext;
class TextureManager;
class RenderScheme;
class World;
struct FRECT;
struct IRender;

class WorldView
{
public:
    WorldView(IRender &render, TextureManager &tm, RenderScheme &rs);
	~WorldView();
	void Render(DrawingContext &dc, World &world, const FRECT &view, bool editorMode) const;

private:
    IRender &_render;
    TextureManager &_tm;
	RenderScheme &_renderScheme;
	Terrain _terrain;
};
