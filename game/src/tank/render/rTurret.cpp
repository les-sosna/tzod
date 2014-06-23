#include "rTurret.h"
#include "gc/Turrets.h"
#include "video/TextureManager.h"

R_Turret::R_Turret(TextureManager &tm, const char *texPlatform, const char *texWeapon)
	: _tm(tm)
	, _texPlatform(tm.FindSprite(texPlatform))
	, _texWeapon(tm.FindSprite(texWeapon))
{
}

void R_Turret::Draw(const GC_Actor &actor, DrawingContext &dc) const
{
	assert(dynamic_cast<const GC_2dSprite*>(&actor));
	auto &turret = static_cast<const GC_Turret&>(actor);
	
	vec2d pos = turret.GetPos();
	vec2d dir = turret.GetDirection();
	vec2d weapDir = vec2d(turret.GetWeaponDir());
	float ready = turret.GetReadyState();
	unsigned int nFrames = _tm.GetFrameCount(_texPlatform);
	if (ready == 1)
	{
		dc.DrawSprite(_texPlatform, nFrames - 1, 0xffffffff, pos.x, pos.y, dir);
		dc.DrawSprite(_texWeapon, 0, 0x40000000, pos.x + 4, pos.y + 4, weapDir); // shadow
		dc.DrawSprite(_texWeapon, 0, 0xffffffff, pos.x, pos.y, weapDir);
	}
	else
	{
		assert(nFrames > 1);
		unsigned int frame = (unsigned int) ((float) (nFrames - 1) * ready);
		dc.DrawSprite(_texPlatform, frame, 0xffffffff, pos.x, pos.y, dir);
	}
}
