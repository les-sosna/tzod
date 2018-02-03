#include "rTile.h"
#include <gc/Actor.h>
#include <gc/NeighborAware.h>
#include <video/TextureManager.h>
#include <video/RenderContext.h>

static const float dx[8]   = { 32, 32,  0,-32,-32,-32,  0, 32 };
static const float dy[8]   = {  0, 32, 32, 32,  0,-32,-32,-32 };
static const int frames[8] = {  5,  8,  7,  6,  3,  0,  1,  2 };

R_Tile::R_Tile(TextureManager &tm, const char *tex, SpriteColor color, vec2d offset, bool anyLOD)
	: _texId(tm.FindSprite(tex))
	, _color(color)
	, _offset(offset)
	, _anyLOD(anyLOD)
{
	assert(9 == tm.GetFrameCount(_texId));
}

void R_Tile::Draw(const World &world, const GC_Actor &actor, RenderContext &rc) const
{
	assert(dynamic_cast<const GI_NeighborAware*>(&actor));

	vec2d pos = actor.GetPos() + _offset;
	vec2d dir = actor.GetDirection();

	if (rc.GetScale() > 0.25 )
	{
		auto &na = dynamic_cast<const GI_NeighborAware&>(actor);
		int tile = na.GetNeighbors(world);
		for (int i = 0; i < 8; ++i)
		{
			if (0 == (tile & (1 << i)))
			{
				rc.DrawSprite(_texId, frames[i], _color, pos + vec2d{ dx[i], dy[i] }, dir);
			}
		}
	}
	if (_anyLOD || rc.GetScale() > 0.25)
	{
		rc.DrawSprite(_texId, 4, _color, pos, dir);
	}
}
