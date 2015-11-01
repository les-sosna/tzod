#include "rFireSpark.h"
#include <gc/projectiles.h>
#include <gc/World.h>
#include <video/TextureManager.h>
#include <video/DrawingContext.h>

R_FireSpark::R_FireSpark(TextureManager &tm)
	: _tm(tm)
	, _texId(tm.FindSprite("projectile_fire"))
{
}

void R_FireSpark::Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const
{
	assert(dynamic_cast<const GC_FireSpark*>(&actor));
	auto &fire = static_cast<const GC_FireSpark&>(actor);

	auto idAsSeed = actor.GetId();
	uint32_t seed = reinterpret_cast<const uint32_t&>(idAsSeed);
	uint32_t rand = ((uint64_t)seed * 279470273UL) % 4294967291UL;

	vec2d pos = fire.GetPos();
	vec2d dir = fire.GetDirection();
	float size = fire.GetRadius();
	unsigned int frame = rand % _tm.GetFrameCount(_texId);
	dc.DrawSprite(_texId, frame, 0xffffffff, pos.x, pos.y, size, size, dir);
}
