#pragma once

#include "Terrain.h"
#include <math/MyMath.h>

class RenderContext;
class TextureManager;
class RenderScheme;
class World;

class WorldView
{
public:
	WorldView(TextureManager &tm, RenderScheme &rs);
	~WorldView();
	void Render(RenderContext &rc,
	            const World &world,
	            const RectRB &viewport,
	            vec2d eye,
	            float zoom,
	            bool editorMode,
	            bool drawGrid,
	            bool nightMode) const;
	RenderScheme &GetRenderScheme() const { return _renderScheme; }

private:
	RenderScheme &_renderScheme;
	Terrain _terrain;
};
