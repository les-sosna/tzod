#pragma once
#include "ObjectView.h"
#include <stddef.h>

class TextureManager;

class R_BrickFragment : public ObjectRFunc
{
public:
	R_BrickFragment(TextureManager &tm);
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;

private:
	TextureManager &_tm;
	size_t _texId;
};
