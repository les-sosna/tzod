#include "rBooster.h"
#include <gc/MovingObject.h>
#include <gc/World.h>
#include <video/TextureManager.h>
#include <video/RenderContext.h>

R_Booster::R_Booster(TextureManager &tm)
	: _texId(tm.FindSprite("booster"))
{
}

void R_Booster::Draw(const World &world, const GC_MovingObject &mo, RenderContext &rc) const
{
	vec2d pos = mo.GetPos();
	vec2d dir = Vec2dDirection(world.GetTime() * 50);
	rc.DrawSprite(_texId, 0, 0xffffffff, pos, dir);
}
