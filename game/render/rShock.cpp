#include "rShock.h"
#include <gc/Pickup.h>
#include <gc/World.h>
#include <video/TextureManager.h>
#include <video/RenderContext.h>

R_Shock::R_Shock(TextureManager &tm)
	: _texId(tm.FindSprite("lightning"))
{
}

void R_Shock::Draw(const World &world, const GC_MovingObject &mo, RenderContext &rc) const
{
	assert(dynamic_cast<const GC_pu_Shock*>(&mo));
	auto &shock = static_cast<const GC_pu_Shock&>(mo);
	if( !shock.GetGridSet() )
	{
		SpriteColor c;
		c.r = c.g = c.b = c.a = int((1.0f - ((world.GetTime() - shock.GetTimeAttached() - SHOCK_TIMEOUT) * 5.0f)) * 255.0f);
		vec2d pos0 = shock.GetPos();
		vec2d pos1 = shock.GetTargetPos();
		rc.DrawLine(_texId, c, pos0, pos1, frand((pos1 - pos0).len()));
	}
}
