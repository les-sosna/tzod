#include "rSprite.h"
#include <gc/MovingObject.h>
#include <video/TextureManager.h>
#include <video/RenderContext.h>

R_Sprite::R_Sprite(TextureManager &tm, const char *tex)
	: _texId(tm.FindSprite(tex))
{
}

void R_Sprite::Draw(const World &world, const GC_MovingObject &mo, RenderContext &rc) const
{
	vec2d pos = mo.GetPos();
	vec2d dir = mo.GetDirection();
	rc.DrawSprite(_texId, 0, 0xffffffff, pos, dir);
}
