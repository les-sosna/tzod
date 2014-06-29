#pragma once
#include "ObjectView.h"

#include <stddef.h>

class TextureManager;

class R_Wall : public ObjectView
{
public:
	R_Wall(TextureManager &tm, const char *tex);
	
	// ObjectView
	virtual enumZOrder GetZ(World &world, const GC_Actor &actor) const { return Z_WALLS; }
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	enum {WALL, LT, RT, RB, LB};
	size_t _texId[5];
	TextureManager &_tm;
};

