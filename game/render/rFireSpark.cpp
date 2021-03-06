#include "rFireSpark.h"
#include <gc/Projectiles.h>
#include <gc/World.h>
#include <video/TextureManager.h>
#include <video/RenderContext.h>

R_FireSpark::R_FireSpark(TextureManager &tm)
	: _tm(tm)
	, _texId(tm.FindSprite("projectile_fire"))
{
}

void R_FireSpark::Draw(const World &world, const GC_MovingObject &mo, RenderContext &rc) const
{
	assert(dynamic_cast<const GC_FireSpark*>(&mo));
	auto &fire = static_cast<const GC_FireSpark&>(mo);

	auto idAsSeed = mo.GetId();
	uint32_t seed = reinterpret_cast<const uint32_t&>(idAsSeed);
	uint32_t rand = ((uint64_t)seed * 279470273UL) % 4294967291UL;

	vec2d pos = fire.GetPos();
	vec2d dir = fire.GetDirection();
	float size = fire.GetRadius();
	unsigned int frame = rand % _tm.GetFrameCount(_texId);
	rc.DrawSprite(_texId, frame, 0xffffffff, pos, size, size, dir);
}
