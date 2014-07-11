#pragma once
#include "ObjectView.h"

#include <stddef.h>

class TextureManager;

class R_AnimatedSprite : public ObjectRFunc
{
public:
	R_AnimatedSprite(TextureManager &tm, const char *tex, float frameRate);
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	TextureManager &_tm;
	size_t _texId;
	float _frameRate;
};
