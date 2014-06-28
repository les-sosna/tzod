#include "rWeapon.h"
#include "gc/Weapons.h"
#include "gc/World.h"
#include "video/TextureManager.h"


static void DrawWeaponShadow(DrawingContext &dc, size_t texId, const GC_Weapon &weapon)
{
	vec2d pos = weapon.GetPos();
	vec2d dir = weapon.GetDirection();
	float shadow = weapon.GetCarrier() ? 2.0f : 4.0f;
	dc.DrawSprite(texId, 0, 0x40000000, pos.x + shadow, pos.y + shadow, dir);	
}


enumZOrder R_WeaponBase::GetZ(const GC_Actor &actor) const
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


R_WeaponMinigun::R_WeaponMinigun(TextureManager &tm)
	: _texId1(tm.FindSprite("weap_mg1"))
	, _texId2(tm.FindSprite("weap_mg2"))
{
}

void R_WeaponMinigun::Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const
{
	assert(dynamic_cast<const GC_Weap_Minigun*>(&actor));
	auto &minigun = static_cast<const GC_Weap_Minigun&>(actor);

	vec2d pos = minigun.GetPos();
	vec2d dir = minigun.GetDirection();
	size_t texId = minigun.GetFire() ? ((fmod(world._time, 0.1f) < 0.05f) ? _texId1 : _texId2) : _texId2;
	DrawWeaponShadow(dc, texId, minigun);	
	dc.DrawSprite(texId, 0, 0xffffffff, pos.x, pos.y, dir);
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

