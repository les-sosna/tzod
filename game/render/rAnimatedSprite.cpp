#include "rAnimatedSprite.h"
#include <gc/MovingObject.h>
#include <gc/World.h>
#include <video/TextureManager.h>
#include <video/RenderContext.h>

R_AnimatedSprite::R_AnimatedSprite(TextureManager &tm, const char *tex, float frameRate)
	: _tm(tm)
	, _texId(tm.FindSprite(tex))
	, _frameRate(frameRate)
{
}

void R_AnimatedSprite::Draw(const World &world, const GC_MovingObject &mo, RenderContext &rc) const
{
	vec2d pos = mo.GetPos();
	vec2d dir = mo.GetDirection();
	unsigned int frame = static_cast<unsigned int>(world.GetTime() * _frameRate) % _tm.GetFrameCount(_texId);
	rc.DrawSprite(_texId, frame, 0xffffffff, pos, dir);
}


R_AnimatedSpriteSequence::R_AnimatedSpriteSequence(TextureManager& tm, const char* tex, float frameRate, std::vector<int> frames)
	: _texId(tm.FindSprite(tex))
	, _frameRate(frameRate)
	, _frames(std::move(frames))
{
}

void R_AnimatedSpriteSequence::Draw(const World& world, const GC_MovingObject& mo, RenderContext& rc) const
{
	vec2d pos = mo.GetPos();
	vec2d dir = mo.GetDirection();
	unsigned int frame = static_cast<unsigned int>(world.GetTime() * _frameRate) % _frames.size();
	rc.DrawSprite(_texId, _frames[frame], 0xffffffff, pos, dir);
}
