#pragma once
#include "ObjectView.h"

#include <stddef.h>

class TextureManager;

class R_AnimatedSprite : public ObjectView
{
public:
	R_AnimatedSprite(TextureManager &tm, const char *tex, enumZOrder z, float frameRate);
	
	// ObjectView
	virtual enumZOrder GetZ(World &world, const GC_Actor &actor) const { return _z; }
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	TextureManager &_tm;
	size_t _texId;
	enumZOrder _z;
	float _frameRate;
};
