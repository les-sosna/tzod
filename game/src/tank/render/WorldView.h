#pragma once

#include "Terrain.h"

class DrawingContext;
class TextureManager;
class RenderScheme;
class World;
struct Rect;
class vec2d;

class WorldView
{
public:
    WorldView(TextureManager &tm, RenderScheme &rs);
	~WorldView();
	void Render(DrawingContext &dc,
				const World &world,
				const Rect &viewport,
				const vec2d &eye,
				float zoom,
				bool editorMode,
				bool drawGrid,
				bool nightMode) const;
	RenderScheme &GetRenderScheme() const { return _renderScheme; }

private:
	RenderScheme &_renderScheme;
	Terrain _terrain;
};
