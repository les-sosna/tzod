#include "rVehicle.h"
#include "config/Config.h"
#include "gc/Vehicle.h"
#include "gc/Player.h"
#include "video/DrawingContext.h"
#include "video/TextureManager.h"

R_Vehicle::R_Vehicle(TextureManager &tm)
	: _tm(tm)
	, _nameFont(tm.FindSprite("font_small"))
{
}
	
void R_Vehicle::Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const
{
	assert(dynamic_cast<const GC_Vehicle*>(&actor));
	auto &vehicle = static_cast<const GC_Vehicle&>(actor);
	
	vec2d pos = vehicle.GetPos();
	vec2d dir = vehicle.GetDirection();
	size_t texId = _tm.FindSprite(vehicle.GetSkin());
	dc.DrawSprite(texId, 0, 0x40000000, pos.x + 4, pos.y + 4, dir);
	dc.DrawSprite(texId, 0, 0xffffffff, pos.x, pos.y, dir);
	
	if( g_conf.g_shownames.Get() && vehicle.GetOwner() )
	{
		dc.DrawBitmapText(floorf(pos.x), floorf(pos.y) + floorf(_tm.GetFrameHeight(texId, 0) / 2),
                          _nameFont, 0x7f7f7f7f, vehicle.GetOwner()->GetNick(), alignTextCB);
	}
	
#ifndef NDEBUG
	//	for( int i = 0; i < 4; ++i )
	//	{
	//		DbgLine(GetVertex(i), GetVertex((i+1)&3));
	//	}
#endif // NDEBUG
}
