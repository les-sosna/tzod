#include "rDecoration.h"
#include <gc/UserObjects.h>
#include <video/RenderContext.h>
#include <video/TextureManager.h>

R_Decoration::R_Decoration(TextureManager &tm)
	: _tm(tm)
{
}

void R_Decoration::Draw(const World &world, const GC_MovingObject &mo, RenderContext &rc) const
{
	assert(dynamic_cast<const GC_Decoration*>(&mo));
	auto &decoration = static_cast<const GC_Decoration&>(mo);

	vec2d pos = decoration.GetPos();
	vec2d dir = decoration.GetDirection();
	size_t texId = _tm.FindSprite(decoration.GetTextureName());
	rc.DrawSprite(texId, 0, 0xffffffff, pos, dir);
}

enumZOrder Z_Decoration::GetZ(const World &world, const GC_MovingObject &mo) const
{
	assert(dynamic_cast<const GC_Decoration*>(&mo));
	auto &decoration = static_cast<const GC_Decoration&>(mo);
	return decoration.GetZ();
}

