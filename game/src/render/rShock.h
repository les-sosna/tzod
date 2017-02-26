#pragma once
#include "inc/render/ObjectView.h"
#include <stddef.h>

class TextureManager;

class R_Shock : public ObjectRFunc
{
public:
	R_Shock(TextureManager &tm);
	void Draw(const World &world, const GC_Actor &actor, RenderContext &rc) const override;
private:
	size_t _texId;
};
