#include "rMinigun.h"
#include "rWeaponBase.h"
#include <gc/Weapons.h>
#include <gc/WeapCfg.h>
#include <gc/World.h>
#include <video/TextureManager.h>
#include <video/RenderContext.h>

R_WeaponMinigun::R_WeaponMinigun(TextureManager &tm)
	: _texId1(tm.FindSprite("weap_mg1"))
	, _texId2(tm.FindSprite("weap_mg2"))
{
}

void R_WeaponMinigun::Draw(const World &world, const GC_MovingObject &mo, RenderContext &rc) const
{
	assert(dynamic_cast<const GC_Weap_Minigun*>(&mo));
	auto &minigun = static_cast<const GC_Weap_Minigun&>(mo);

	vec2d pos = minigun.GetPos();
	vec2d dir = GetWeapSpriteDirection(world, minigun);
	size_t texId = minigun.GetFire() ? ((fmod(world.GetTime(), 0.1f) < 0.05f) ? _texId1 : _texId2) : _texId2;
	DrawWeaponShadow(world, minigun, rc, texId);
	rc.DrawSprite(texId, 0, 0xffffffff, pos, dir);
}


R_Crosshair2::R_Crosshair2(TextureManager &tm)
	: _texId(tm.FindSprite("indicator_crosshair2"))
{
}

void R_Crosshair2::Draw(const World &world, const GC_MovingObject &mo, RenderContext &rc) const
{
	assert(dynamic_cast<const GC_Weap_Minigun*>(&mo));
	auto &minigun = static_cast<const GC_Weap_Minigun&>(mo);
	if (minigun.GetAttached())
	{
		vec2d delta = Vec2dDirection(minigun.GetHeat(world) * 0.1f / WEAP_MG_TIME_RELAX);
		vec2d dir1 = Vec2dAddDirection(minigun.GetDirection(), delta);
		vec2d dir2 = Vec2dSubDirection(minigun.GetDirection(), delta);
		vec2d pos1 = minigun.GetPos() + dir1 * 150.0f;
		vec2d pos2 = minigun.GetPos() + dir2 * 150.0f;
		rc.DrawSprite(_texId, 0, 0xffffffff, pos1, dir1);
		rc.DrawSprite(_texId, 0, 0xffffffff, pos2, dir2);
	}
}
