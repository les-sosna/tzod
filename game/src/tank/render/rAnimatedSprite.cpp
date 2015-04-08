#include "rAnimatedSprite.h"
#include <gc/Actor.h>
#include <gc/World.h>
#include <video/TextureManager.h>
#include <video/DrawingContext.h>

R_AnimatedSprite::R_AnimatedSprite(TextureManager &tm, const char *tex, float frameRate)
	: _tm(tm)
	, _texId(tm.FindSprite(tex))
	, _frameRate(frameRate)
{
}
	
void R_AnimatedSprite::Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const
{
	vec2d pos = actor.GetPos();
	vec2d dir = actor.GetDirection();
	size_t frame = size_t(world.GetTime() * _frameRate) % _tm.GetFrameCount(_texId);
	dc.DrawSprite(_texId, frame, 0xffffffff, pos.x, pos.y, dir);
}
