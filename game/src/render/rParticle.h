#pragma once
#include "inc/render/ObjectView.h"
#include <stddef.h>
#include <vector>

class TextureManager;

class R_Particle : public ObjectRFunc
{
public:
	R_Particle(TextureManager &tm);
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;

private:
	TextureManager &_tm;
	std::vector<size_t> _ptype2texId;
};
