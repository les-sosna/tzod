#pragma once

#include "Terrain.h"

class DrawingContext;
class TextureManager;
class RenderScheme;
class World;
struct Rect;
struct vec2d;
struct IRender;

class WorldView
{
public:
    WorldView(IRender &render, TextureManager &tm, RenderScheme &rs);
	~WorldView();
	void Render(DrawingContext &dc, World &world, const Rect &viewport, const vec2d &eye, float zoom, bool editorMode) const;

private:
    IRender &_render;
	RenderScheme &_renderScheme;
	Terrain _terrain;
};
