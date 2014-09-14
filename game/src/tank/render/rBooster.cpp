#include "rBooster.h"
#include "gc/Actor.h"
#include "gc/World.h"
#include <video/TextureManager.h>
#include <video/DrawingContext.h>

R_Booster::R_Booster(TextureManager &tm)
	: _texId(tm.FindSprite("booster"))
{
}
	
void R_Booster::Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const
{
	vec2d pos = actor.GetPos();
	vec2d dir = vec2d(world.GetTime() * 50);
	dc.DrawSprite(_texId, 0, 0xffffffff, pos.x, pos.y, dir);
}
