#include "rWeaponBase.h"
#include "gc/Weapons.h"
#include <video/DrawingContext.h>

void DrawWeaponShadow(DrawingContext &dc, size_t texId, const GC_Weapon &weapon)
{
	vec2d pos = weapon.GetPos();
	vec2d dir = weapon.GetDirection();
	float shadow = weapon.GetCarrier() ? 2.0f : 4.0f;
	dc.DrawSprite(texId, 0, 0x40000000, pos.x + shadow, pos.y + shadow, dir);	
}


enumZOrder Z_Weapon::GetZ(const World &world, const GC_Actor &actor) const
{
	assert(dynamic_cast<const GC_Weapon*>(&actor));
	auto &weapon = static_cast<const GC_Weapon&>(actor);
	if( weapon.GetCarrier() )
	{
		return Z_ATTACHED_ITEM;
	}
	else
	{
		bool blinking = weapon.GetRespawn() && (weapon._time > weapon._timeStay - 3.0f);
		bool visible = weapon.GetVisible() && (!blinking || fmod(weapon._time, 0.16f) > 0.08f); // or editorMode
		return visible ? Z_FREE_ITEM : Z_NONE;
	}
}
