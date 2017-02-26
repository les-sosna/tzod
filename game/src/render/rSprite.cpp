#include "rSprite.h"
#include <gc/Actor.h>
#include <video/TextureManager.h>
#include <video/RenderContext.h>

R_Sprite::R_Sprite(TextureManager &tm, const char *tex)
	: _texId(tm.FindSprite(tex))
{
}

void R_Sprite::Draw(const World &world, const GC_Actor &actor, RenderContext &rc) const
{
	vec2d pos = actor.GetPos();
	vec2d dir = actor.GetDirection();
	rc.DrawSprite(_texId, 0, 0xffffffff, pos.x, pos.y, dir);
}
