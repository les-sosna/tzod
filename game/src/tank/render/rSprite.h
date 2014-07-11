#pragma once
#include "ObjectView.h"

#include <stddef.h>

class TextureManager;

class R_Sprite : public ObjectView
{
public:
	R_Sprite(TextureManager &tm, const char *tex, enumZOrder z);
	
	// ObjectView
	virtual enumZOrder GetZ(const World &world, const GC_Actor &actor) const { return _z; }
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	size_t _texId;
	enumZOrder _z;
};
