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

void R_Vehicle::Draw(const World &world, const GC_MovingObject &mo, RenderContext &rc) const
{
	assert(dynamic_cast<const GC_Vehicle*>(&mo));
	auto &vehicle = static_cast<const GC_Vehicle&>(mo);

	vec2d pos = vehicle.GetPos();
	vec2d dir = vehicle.GetDirection();

	size_t texId = _tm.FindSprite(vehicle.GetSkin());
	auto frameCount = _tm.GetFrameCount(texId);

	if( frameCount == 1 )
	{
		rc.DrawSprite(texId, 0, 0x40000000, pos + vec2d{ 4, 4 }, dir);
		rc.DrawSprite(texId, 0, 0xffffffff, pos, dir);
	}
	else
	{
		float vehicleHalfWidth = _tm.GetFrameHeight(texId, 0);
		int leftFrame = ((int)vehicle._trackTotalPathL % (frameCount / 2)) * 2;
		int rightFrame = ((int)vehicle._trackTotalPathR % (frameCount / 2)) * 2 + 1;

		auto leftOffset = vec2d{ dir.y, -dir.x } * vehicleHalfWidth / 2;
		auto rightOffset = vec2d{ -dir.y, dir.x } * vehicleHalfWidth / 2;
		rc.DrawSprite(texId, leftFrame, 0x40000000, leftOffset + pos + vec2d{ 4, 4 }, dir);
		rc.DrawSprite(texId, rightFrame, 0x40000000, rightOffset + pos + vec2d{ 4, 4 }, dir);
		rc.DrawSprite(texId, leftFrame, 0xffffffff, leftOffset + pos, dir);
		rc.DrawSprite(texId, rightFrame, 0xffffffff, rightOffset + pos, dir);
	}

	if( vehicle.GetOwner() )
	{
		float radius = vehicle.GetRadius();
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
