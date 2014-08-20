#include "rAnimatedSprite.h"
#include "gc/2dSprite.h"
#include "gc/World.h"
#include "video/TextureManager.h"
#include "video/DrawingContext.h"

R_AnimatedSprite::R_AnimatedSprite(TextureManager &tm, const char *tex, float frameRate)
	: _tm(tm)
	, _texId(tm.FindSprite(tex))
	, _frameRate(frameRate)
{
}
	
void R_AnimatedSprite::Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const
{
	assert(dynamic_cast<const GC_2dSprite*>(&actor));
	auto &sprite = static_cast<const GC_2dSprite&>(actor);
	
	vec2d pos = sprite.GetPos();
	vec2d dir = sprite.GetDirection();
	size_t frame = size_t(world.GetTime() * _frameRate) % _tm.GetFrameCount(_texId);
	dc.DrawSprite(_texId, frame, 0xffffffff, pos.x, pos.y, dir);
}
