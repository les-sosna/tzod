#include "rIndicator.h"
#include <gc/Vehicle.h>
#include <gc/Weapons.h>
#include <gc/World.h>
#include <video/TextureManager.h>
#include <video/RenderContext.h>

R_HealthIndicator::R_HealthIndicator(TextureManager &tm, bool dynamic)
	: _tm(tm)
	, _texId(tm.FindSprite("indicator_health"))
	, _dynamic(dynamic)
{
}

void R_HealthIndicator::Draw(const World &world, const GC_Actor &actor, RenderContext &rc) const
{
	assert(dynamic_cast<const GC_RigidBodyStatic*>(&actor));
	auto &rigidBody = static_cast<const GC_RigidBodyStatic&>(actor);

	vec2d pos = rigidBody.GetPos();
	float radius = _dynamic ? rigidBody.GetRadius() : rigidBody.GetHalfWidth();
	float val = rigidBody.GetHealth() / rigidBody.GetHealthMax();
	rc.DrawIndicator(_texId, { pos.x, pos.y - radius - _tm.GetFrameHeight(_texId, 0) }, val);
}


static void DrawWeaponIndicator(const World &world,
								const TextureManager &tm,
								RenderContext &rc,
								size_t texId,
								const GC_Weapon &weapon,
								float value)
{
	if( GC_Vehicle *vehicle = weapon.GetVehicle() )
	{
		vec2d pos = vehicle->GetPos();
		float radius = vehicle->GetRadius();
		rc.DrawIndicator(texId, { pos.x, pos.y + radius }, value);
	}
}


R_AmmoIndicator::R_AmmoIndicator(TextureManager &tm)
	: _tm(tm)
	, _texId(tm.FindSprite("indicator_ammo"))
{
}

void R_AmmoIndicator::Draw(const World &world, const GC_Actor &actor, RenderContext &rc) const
{
	assert(dynamic_cast<const GC_ProjectileBasedWeapon*>(&actor));
	auto &weapon = static_cast<const GC_ProjectileBasedWeapon&>(actor);
	float value = 1 - (float) weapon.GetNumShots() / (float) weapon.GetSeriesLength();
	DrawWeaponIndicator(world, _tm, rc, _texId, weapon, value);
}


R_FuelIndicator::R_FuelIndicator(TextureManager &tm)
	: _tm(tm)
	, _texId(tm.FindSprite("indicator_fuel"))
{
}

void R_FuelIndicator::Draw(const World &world, const GC_Actor &actor, RenderContext &rc) const
{
	assert(dynamic_cast<const GC_Weap_Ram*>(&actor));
	auto &ram = static_cast<const GC_Weap_Ram&>(actor);
	DrawWeaponIndicator(world, _tm, rc, _texId, ram, ram.GetFuel() / ram.GetFuelMax());
}
