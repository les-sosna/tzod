#pragma once
#include "inc/render/ObjectView.h"
#include <stddef.h>

class TextureManager;

class R_BrickFragment : public ObjectRFunc
{
public:
	R_BrickFragment(TextureManager &tm);
	void Draw(const World &world, const GC_Actor &actor, RenderContext &rc) const override;

private:
	TextureManager &_tm;
	size_t _texId;
};
