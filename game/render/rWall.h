#pragma once
#include "inc/render/ObjectView.h"
#include <stddef.h>

class TextureManager;

class R_Wall : public ObjectRFunc
{
public:
	R_Wall(TextureManager &tm, const char *tex);
	void Draw(const World &world, const GC_MovingObject &mo, RenderContext &rc) const override;

private:
	enum {WALL, LT, RT, RB, LB};
	size_t _texId[5];
	TextureManager &_tm;
};

