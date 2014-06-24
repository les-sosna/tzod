#include "rSprite.h"
#include "gc/2dSprite.h"
#include "video/TextureManager.h"

R_Sprite::R_Sprite(TextureManager &tm, const char *tex, enumZOrder z)
	: _texId(tm.FindSprite(tex))
	, _z(z)
{
}
	
void R_Sprite::Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const
{
	assert(dynamic_cast<const GC_2dSprite*>(&actor));
	auto &sprite = static_cast<const GC_2dSprite&>(actor);
	
	vec2d pos = sprite.GetPos();
	vec2d dir = sprite.GetDirection();
	dc.DrawSprite(_texId, 0, 0xffffffff, pos.x, pos.y, dir);
}
