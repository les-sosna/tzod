#include "rVehicle.h"
#include <gc/Vehicle.h>
#include <gc/Player.h>
#include <video/RenderContext.h>
#include <video/TextureManager.h>

R_Vehicle::R_Vehicle(TextureManager &tm)
	: _tm(tm)
	, _nameFont(tm.FindSprite("font_small"))
{
}

void R_Vehicle::Draw(const World &world, const GC_Actor &actor, RenderContext &rc) const
{
	assert(dynamic_cast<const GC_Vehicle*>(&actor));
	auto &vehicle = static_cast<const GC_Vehicle&>(actor);

	vec2d pos = vehicle.GetPos();
	vec2d dir = vehicle.GetDirection();
	float radius = vehicle.GetRadius();
	size_t texId = _tm.FindSprite(vehicle.GetSkin());
	rc.DrawSprite(texId, 0, 0x40000000, pos + vec2d{ 4, 4 }, dir);
	rc.DrawSprite(texId, 0, 0xffffffff, pos, dir);

	if( vehicle.GetOwner() )
	{
		rc.DrawBitmapText(vec2d{ pos.x, pos.y + radius + 4 }, // leave space for ammo indicator
			1.f, _nameFont, 0x7f7f7f7f, vehicle.GetOwner()->GetNick(), alignTextCT);
	}

#ifndef NDEBUG
	//	for( int i = 0; i < 4; ++i )
	//	{
	//		DbgLine(GetVertex(i), GetVertex((i+1)&3));
	//	}
#endif // NDEBUG
}
