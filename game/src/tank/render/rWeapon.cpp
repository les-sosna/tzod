#include "rWeapon.h"
#include "gc/Weapons.h"
#include "video/TextureManager.h"

R_Weapon::R_Weapon(TextureManager &tm, const char *tex)
	: _texId(tm.FindSprite(tex))
{
}

enumZOrder R_Weapon::GetZ(const GC_Actor &actor) const
{
	assert(dynamic_cast<const GC_2dSprite*>(&actor));
	auto &weapon = static_cast<const GC_Weapon&>(actor);
	return weapon.GetCarrier() ? Z_ATTACHED_ITEM : Z_FREE_ITEM;
}

void R_Weapon::Draw(const GC_Actor &actor, DrawingContext &dc) const
{
	assert(dynamic_cast<const GC_2dSprite*>(&actor));
	auto &weapon = static_cast<const GC_Weapon&>(actor);
	
	vec2d pos = weapon.GetPos();
	vec2d dir = weapon.GetDirection();
	float shadow = weapon.GetCarrier() ? 2 : 4;
	dc.DrawSprite(_texId, 0, 0x40000000, pos.x + shadow, pos.y + shadow, dir);
	dc.DrawSprite(_texId, 0, 0xffffffff, pos.x, pos.y, dir);
}
