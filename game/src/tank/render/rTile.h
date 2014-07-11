#pragma once
#include "ObjectView.h"
#include <stddef.h>

class TextureManager;

class R_Tile : public ObjectRFunc
{
public:
	R_Tile(TextureManager &tm, const char *tex);
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	size_t _texId;
};
