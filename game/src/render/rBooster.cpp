#include "rBooster.h"
#include <gc/Actor.h>
#include <gc/World.h>
#include <video/TextureManager.h>
#include <video/RenderContext.h>

R_Booster::R_Booster(TextureManager &tm)
	: _texId(tm.FindSprite("booster"))
{
}

void R_Booster::Draw(const World &world, const GC_Actor &actor, RenderContext &rc) const
{
	vec2d pos = actor.GetPos();
	vec2d dir = Vec2dDirection(world.GetTime() * 50);
	rc.DrawSprite(_texId, 0, 0xffffffff, pos, dir);
}
