#pragma once
#include "inc/render/ObjectView.h"
#include <video/RenderBase.h>
#include <stddef.h>

class TextureManager;

class R_Tile : public ObjectRFunc
{
public:
	R_Tile(TextureManager &tm, const char *tex, SpriteColor color, vec2d offset, bool anyLOD);
	void Draw(const World &world, const GC_Actor &actor, RenderContext &rc) const override;

private:
	size_t _texId;
	SpriteColor _color;
	vec2d _offset;
	bool _anyLOD;
	bool _animated;
};
