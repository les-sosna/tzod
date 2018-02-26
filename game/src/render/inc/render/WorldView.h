#pragma once

#include "Terrain.h"
#include <math/MyMath.h>

class AIManager;
class RenderContext;
class TextureManager;
class RenderScheme;
class World;

struct WorldViewRenderOptions
{
	bool nightMode = false;
	bool editorMode = false;
	bool drawGrid = false;
	bool visualizeField = false;
	bool visualizePath = false;
};

class WorldView
{
public:
	WorldView(TextureManager &tm, RenderScheme &rs);
	~WorldView();
	void Render(RenderContext &rc,
	            const World &world,
	            WorldViewRenderOptions options = {},
	            const AIManager *aiManager = nullptr) const;
	RenderScheme &GetRenderScheme() const { return _renderScheme; }

private:
	RenderScheme &_renderScheme;
	Terrain _terrain;
	size_t _lineTex;
	size_t _texField;
};

vec2d ComputeWorldTransformOffset(const FRECT &canvasViewport, vec2d eye, float zoom);
