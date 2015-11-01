#pragma once
#include "inc/render/ObjectView.h"
#include <stddef.h>

class TextureManager;

class R_FireSpark : public ObjectRFunc
{
public:
	R_FireSpark(TextureManager &tm);
	void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;

private:
	TextureManager &_tm;
	size_t _texId;
};
