#include "rDecoration.h"
#include "gc/UserObjects.h"
#include <video/DrawingContext.h>
#include <video/TextureManager.h>

R_Decoration::R_Decoration(TextureManager &tm)
	: _tm(tm)
{
}
	
void R_Decoration::Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const
{
	assert(dynamic_cast<const GC_Decoration*>(&actor));
	auto &decoration = static_cast<const GC_Decoration&>(actor);
	
	vec2d pos = decoration.GetPos();
	vec2d dir = decoration.GetDirection();
	size_t texId = _tm.FindSprite(decoration.GetTextureName());
	dc.DrawSprite(texId, 0, 0xffffffff, pos.x, pos.y, dir);
}

enumZOrder Z_Decoration::GetZ(const World &world, const GC_Actor &actor) const
{
	assert(dynamic_cast<const GC_Decoration*>(&actor));
	auto &decoration = static_cast<const GC_Decoration&>(actor);
	return decoration.GetZ();
}

