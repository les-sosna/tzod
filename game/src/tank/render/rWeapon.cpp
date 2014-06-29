#include "rWeapon.h"
#include "gc/Weapons.h"
#include "gc/World.h"
#include "video/TextureManager.h"

R_Weapon::R_Weapon(TextureManager &tm, const char *tex)
	: _texId(tm.FindSprite(tex))
{
}

void R_Weapon::Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const
{
	assert(dynamic_cast<const GC_Weapon*>(&actor));
	auto &weapon = static_cast<const GC_Weapon&>(actor);

	DrawWeaponShadow(dc, _texId, weapon);	
	vec2d pos = weapon.GetPos();
	vec2d dir = weapon.GetDirection();
	dc.DrawSprite(_texId, 0, 0xffffffff, pos.x, pos.y, dir);
}


R_WeapFireEffect::R_WeapFireEffect(TextureManager &tm, const char *tex, float duration, float offset)
	: _tm(tm)
	, _texId(tm.FindSprite(tex))
{
}

enumZOrder R_WeapFireEffect::GetZ(const GC_Actor &actor) const
{
	return Z_EXPLODE;
}

void R_WeapFireEffect::Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const
{
	assert(dynamic_cast<const GC_Weapon*>(&actor));
	auto &weapon = static_cast<const GC_Weapon&>(actor);
}


R_RipperDisk::R_RipperDisk(TextureManager &tm)
	: _texId(tm.FindSprite("projectile_disk"))
{
}

void R_RipperDisk::Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const
{
	assert(dynamic_cast<const GC_Weap_Ripper*>(&actor));
	auto &ripper = static_cast<const GC_Weap_Ripper&>(actor);
	if (ripper.IsReady())
	{
		vec2d pos = ripper.GetPos() - ripper.GetDirection() * 8;
		vec2d dir = vec2d(world.GetTime() * 10);
		dc.DrawSprite(_texId, 0, 0xffffffff, pos.x, pos.y, dir);
	}
}


R_Crosshair::R_Crosshair(TextureManager &tm)
	: _texId(tm.FindSprite("indicator_crosshair1"))
{
}

void R_Crosshair::Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const
{
	assert(dynamic_cast<const GC_Weapon*>(&actor));
	auto &weapon = static_cast<const GC_Weapon&>(actor);
	if (weapon.GetCarrier())
	{
		vec2d pos = weapon.GetPos() + weapon.GetDirection() * 200.0f;
		vec2d dir = vec2d(world.GetTime() * 5);
		dc.DrawSprite(_texId, 0, 0xffffffff, pos.x, pos.y, dir);
	}	
}
