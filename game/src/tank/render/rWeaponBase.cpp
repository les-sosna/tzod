#include "rWeaponBase.h"
#include "gc/Weapons.h"
#include "gc/World.h"
#include <video/DrawingContext.h>

vec2d GetWeapSpriteDirection(const World &world, const GC_Weapon &weapon)
{
	bool animate = !weapon.GetCarrier() && !weapon.GetRespawn();
	vec2d dir = animate ? vec2d(world.GetTime()) : weapon.GetDirection();
	return dir;
}

void DrawWeaponShadow(const World &world, const GC_Weapon &weapon, DrawingContext &dc, size_t texId)
{
	vec2d pos = weapon.GetPos();
	vec2d dir = GetWeapSpriteDirection(world, weapon);
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
		bool blinking = weapon.GetRespawn() && (world.GetTime() - weapon.GetDetachedTime() > weapon.GetStayTimeout() - 3.0f);
		bool visible = weapon.GetVisible() && (!blinking || fmod(world.GetTime(), 0.16f) > 0.08f); // or editorMode
		return visible ? Z_FREE_ITEM : Z_NONE;
	}
}
