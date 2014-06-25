#pragma once
#include "ObjectView.h"

#include <stddef.h>

class TextureManager;

class R_Tile : public ObjectView
{
public:
	R_Tile(TextureManager &tm, const char *tex, enumZOrder z);
	
	// ObjectView
	virtual enumZOrder GetZ(const GC_Actor &actor) const { return _z; }
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	size_t _texId;
	enumZOrder _z;
};
