#include "rWeapon.h"
#include "rWeaponBase.h"
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


R_WeapFireEffect::R_WeapFireEffect(TextureManager &tm, const char *tex, float duration, float offsetX, bool oriented)
	: _tm(tm)
	, _texId(tm.FindSprite(tex))
	, _duration(duration)
	, _offsetX(offsetX)
	, _oriented(oriented)
{
}

void R_WeapFireEffect::Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const
{
	assert(dynamic_cast<const GC_Weapon*>(&actor));
	auto &weapon = static_cast<const GC_Weapon&>(actor);
	
	float lastShot = weapon.GetLastShotTimestamp();
	float advance = (world.GetTime() - lastShot) / _duration;
	if (advance < 1)
	{
		int frame = int(advance * (float) _tm.GetFrameCount(_texId));
		unsigned char op = (unsigned char) int(255.0f * (1.0f - advance * advance));
		SpriteColor color = { op, op, op, op };
		vec2d pos = weapon.GetPos() + weapon.GetDirection() * _offsetX;
		pos += Vec2dAddDirection(weapon.GetDirection(), vec2d(0, -1)) * weapon.GetFirePointOffset();
		vec2d dir;
		if( _oriented )
		{
			dir = weapon.GetDirection();
		}
		else
		{
			unsigned int hash = reinterpret_cast<unsigned int&>(lastShot);
			hash ^= (hash >> 16) | (hash << 16);
			dir = vec2d((float) hash);
		}
		dc.DrawSprite(_texId, frame, color, pos.x, pos.y, dir);
	}
}

enumZOrder Z_WeapFireEffect::GetZ(const World &world, const GC_Actor &actor) const
{
	assert(dynamic_cast<const GC_Weapon*>(&actor));
	auto &weapon = static_cast<const GC_Weapon&>(actor);
	return (weapon.GetCarrier() && (world.GetTime() - weapon.GetLastShotTimestamp() < _duration)) ? Z_EXPLODE : Z_NONE;
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