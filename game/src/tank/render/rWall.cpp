#include "rWall.h"

#include "gc/RigidBody.h"
#include "video/TextureManager.h"

R_Wall::R_Wall(TextureManager &tm, const char *tex)
	: _tm(tm)
{
	_texId[WALL] = tm.FindSprite(std::string(tex) + "_wall");
	_texId[LT] = tm.FindSprite(std::string(tex) + "_lt");
	_texId[RT] = tm.FindSprite(std::string(tex) + "_rt");
	_texId[RB] = tm.FindSprite(std::string(tex) + "_rb");
	_texId[LB] = tm.FindSprite(std::string(tex) + "_lb");
}

void R_Wall::Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const
{
	assert(dynamic_cast<const GC_Wall*>(&actor));
	auto &wall = static_cast<const GC_Wall&>(actor);
	vec2d pos = wall.GetPos();
	vec2d dir = wall.GetDirection();
	unsigned int corner = wall.GetCorner();
	assert(corner < 5);
	unsigned int fcount = _tm.GetFrameCount(_texId[corner]);
	unsigned int frame = fcount - 1 - (unsigned int) ((float) (fcount - 1) * wall.GetHealth() / wall.GetHealthMax());
	dc.DrawSprite(_texId[corner], frame, 0xffffffff, pos.x, pos.y, dir);
}


