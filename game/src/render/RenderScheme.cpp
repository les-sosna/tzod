#include "inc/render/RenderScheme.h"
#include "rAnimatedSprite.h"
#include "rBrickFragment.h"
#include "rBooster.h"
#include "rDecoration.h"
#include "rFireSpark.h"
#include "rIndicator.h"
#include "rLight.h"
#include "rMinigun.h"
#include "rParticle.h"
#include "rPredicate.h"
#include "rShock.h"
#include "rSprite.h"
#include "rText.h"
#include "rTile.h"
#include "rTurret.h"
#include "rUserObject.h"
#include "rVehicle.h"
#include "rWall.h"
#include "rWeapon.h"
#include "rWeaponBase.h"
#include "RenderCfg.h"
#include <gc/Crate.h>
#include <gc/GameClasses.h>
#include <gc/Indicators.h>
#include <gc/Light.h>
#include <gc/Particles.h>
#include <gc/Projectiles.h>
#include <gc/RigidBody.h>
#include <gc/Trigger.h>
#include <gc/Turrets.h>
#include <gc/UserObjects.h>
#include <gc/Vehicle.h>
#include <gc/Wall.h>
#include <gc/Water.h>
#include <gc/Weapons.h>
#include <gc/World.h>

static bool HasBooster(const World &world, const GC_Actor &actor)
{
	assert(dynamic_cast<const GC_Weapon*>(&actor));
	return nullptr != static_cast<const GC_Weapon&>(actor).GetBooster();
}
static bool IsPickupVisible(const World &world, const GC_Actor &actor)
{
	assert(dynamic_cast<const GC_Pickup*>(&actor));
	return static_cast<const GC_Pickup&>(actor).GetVisible();
}
static bool IsPickupAttached(const World &world, const GC_Actor &actor)
{
	assert(dynamic_cast<const GC_Pickup*>(&actor));
	return static_cast<const GC_Pickup&>(actor).GetAttached();
}

template <class T, class ...Args>
static std::unique_ptr<T> Make(Args && ...args)
{
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

class GameViewsBuilder : public ObjectViewsSelectorBuilder
{
public:
	GameViewsBuilder(TextureManager &tm)
	{
		AddView<GC_Wall>(Make<Z_Const>(Z_WALLS), Make<R_Wall>(tm, "brick"));
		AddView<GC_Wall_Concrete>(Make<Z_Const>(Z_WALLS), Make<R_Wall>(tm, "concrete"));

		AddView<GC_Crate>(Make<Z_Const>(Z_WALLS), Make<R_Sprite>(tm, "crate01"));

		AddView<GC_TankBullet>(Make<Z_Const>(Z_PROJECTILE), Make<R_Sprite>(tm, "projectile_cannon"));
		AddView<GC_Rocket>(Make<Z_Const>(Z_PROJECTILE), Make<R_Sprite>(tm, "projectile_rocket"));
//		AddView<GC_Bullet>(Make<Z_Const>(Z_PROJECTILE), Make<R_Sprite>(tm, "projectile_bullet"));
		AddView<GC_PlazmaClod>(Make<Z_Const>(Z_PROJECTILE), Make<R_Sprite>(tm, "projectile_plazma"));
		AddView<GC_BfgCore>(Make<Z_Const>(Z_PROJECTILE), Make<R_AnimatedSprite>(tm, "projectile_bfg", 10.0f));
		AddView<GC_FireSpark>(Make<Z_Const>(Z_PROJECTILE), Make<R_FireSpark>(tm));
		AddView<GC_ACBullet>(Make<Z_Const>(Z_PROJECTILE), Make<R_Sprite>(tm, "projectile_ac"));
		AddView<GC_Disk>(Make<Z_Const>(Z_PROJECTILE), Make<R_Sprite>(tm, "projectile_disk"));

		AddView<GC_Spotlight>(Make<Z_Const>(Z_PROJECTILE), Make<R_Sprite>(tm, "spotlight"));

		AddView<GC_Tank_Light>(Make<Z_Const>(Z_VEHICLES), Make<R_Vehicle>(tm));
		AddView<GC_Tank_Light>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_HealthIndicator>(tm, true));

		AddView<GC_Text_ToolTip>(Make<Z_Const>(Z_PARTICLE), Make<R_Text>(tm));

		AddView<GC_TurretCannon>(Make<Z_Const>(Z_WALLS), Make<R_Turret>(tm, "turret_platform", "turret_cannon"));
		AddView<GC_TurretCannon>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_HealthIndicator>(tm, false));
		AddView<GC_TurretRocket>(Make<Z_Const>(Z_WALLS), Make<R_Turret>(tm, "turret_platform", "turret_rocket"));
		AddView<GC_TurretRocket>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_HealthIndicator>(tm, false));
		AddView<GC_TurretMinigun>(Make<Z_Const>(Z_WALLS), Make<R_Turret>(tm, "turret_mg_wake", "turret_mg"));
		AddView<GC_TurretMinigun>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_HealthIndicator>(tm, false));
		AddView<GC_TurretGauss>(Make<Z_Const>(Z_WALLS), Make<R_Turret>(tm, "turret_gauss_wake", "turret_gauss"));
		AddView<GC_TurretGauss>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_HealthIndicator>(tm, false));

		AddView<GC_Weap_RocketLauncher>(Make<Z_Weapon>(), Make<R_Weapon>(tm, "weap_ak47"));
		AddView<GC_Weap_RocketLauncher>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_Crosshair>(tm));
		AddView<GC_Weap_RocketLauncher>(Make<Z_WeapFireEffect>(0.1f),
		                                Make<R_WeapFireEffect>(tm, "particle_fire3", 0.1f, 13.0f, true));
		AddView<GC_Weap_AutoCannon>(Make<Z_Weapon>(), Make<R_Weapon>(tm, "weap_ac"));
		AddView<GC_Weap_AutoCannon>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_Crosshair>(tm));
		AddView<GC_Weap_AutoCannon>(Make<Z_Predicate<Z_WeapFireEffect>>(HasBooster, 0.135f),
		                            Make<R_WeapFireEffect>(tm, "particle_fire4", 0.135f, 17.0f, true));
		AddView<GC_Weap_AutoCannon>(Make<Z_Predicate<Z_WeapFireEffect>>(Not(HasBooster), 0.135f),
		                            Make<R_WeapFireEffect>(tm, "particle_fire3", 0.135f, 17.0f, true));
		AddView<GC_Weap_AutoCannon>(Make<Z_Predicate<Z_Const>>(Not(HasBooster), Z_VEHICLE_LABEL),
		                            Make<R_AmmoIndicator>(tm));
		AddView<GC_Weap_Cannon>(Make<Z_Weapon>(), Make<R_Weapon>(tm, "weap_cannon"));
		AddView<GC_Weap_Cannon>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_Crosshair>(tm));
		AddView<GC_Weap_Cannon>(Make<Z_WeapFireEffect>(0.2f),
		                        Make<R_WeapFireEffect>(tm, "particle_fire3", 0.2f, 21.0f, true));
		AddView<GC_Weap_Plazma>(Make<Z_Weapon>(), Make<R_Weapon>(tm, "weap_plazma"));
		AddView<GC_Weap_Plazma>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_Crosshair>(tm));
		AddView<GC_Weap_Plazma>(Make<Z_WeapFireEffect>(0.2f),
		                        Make<R_WeapFireEffect>(tm, "particle_plazma_fire", 0.2f, 0.0f, true));
		AddView<GC_Weap_Gauss>(Make<Z_Weapon>(), Make<R_Weapon>(tm, "weap_gauss"));
		AddView<GC_Weap_Gauss>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_Crosshair>(tm));
		AddView<GC_Weap_Gauss>(Make<Z_WeapFireEffect>(0.15f),
		                       Make<R_WeapFireEffect>(tm, "particle_gaussfire", 0.15f, 0.0f, true));
		AddView<GC_Weap_Ram>(Make<Z_Weapon>(), Make<R_Weapon>(tm, "weap_ram"));
		AddView<GC_Weap_Ram>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_Crosshair>(tm));
		AddView<GC_Weap_Ram>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_FuelIndicator>(tm));
		AddView<GC_Weap_BFG>(Make<Z_Weapon>(), Make<R_Weapon>(tm, "weap_bfg"));
		AddView<GC_Weap_BFG>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_Crosshair>(tm));
		AddView<GC_Weap_Ripper>(Make<Z_Weapon>(), Make<R_Weapon>(tm, "weap_ripper"));
		AddView<GC_Weap_Ripper>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_Crosshair>(tm));
		AddView<GC_Weap_Ripper>(Make<Z_Const>(Z_PROJECTILE), Make<R_RipperDisk>(tm));
		AddView<GC_Weap_Minigun>(Make<Z_Weapon>(), Make<R_WeaponMinigun>(tm));
		AddView<GC_Weap_Minigun>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_Crosshair2>(tm));
		AddView<GC_Weap_Minigun>(Make<Z_Predicate<Z_WeapFireEffect>>(HasBooster, 0.1f),
		                         Make<R_WeapFireEffect>(tm, "particle_fire3", 0.1f, 17.0f, true));
		AddView<GC_Weap_Minigun>(Make<Z_WeapFireEffect>(0.1f),
		                         Make<R_WeapFireEffect>(tm, "minigun_fire", 0.1f, 20.0f, false));
		AddView<GC_Weap_Zippo>(Make<Z_Weapon>(), Make<R_Weapon>(tm, "weap_zippo"));
		AddView<GC_Weap_Zippo>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_Crosshair>(tm));

		AddView<GC_pu_Health>(Make<Z_Predicate<Z_Const>>(IsPickupVisible, Z_FREE_ITEM),
		                      Make<R_AnimatedSprite>(tm, "pu_health", ANIMATION_FPS));
		AddView<GC_pu_Mine>(Make<Z_Const>(Z_FREE_ITEM), Make<R_Sprite>(tm, "item_mine"));
		AddView<GC_pu_Shield>(Make<Z_Predicate<Z_Const>>(And(IsPickupVisible, IsPickupAttached), Z_PARTICLE),
		                      Make<R_AnimatedSprite>(tm, "shield", ANIMATION_FPS));
		AddView<GC_pu_Shield>(Make<Z_Predicate<Z_Const>>(And(IsPickupVisible, Not(IsPickupAttached)), Z_FREE_ITEM),
		                      Make<R_AnimatedSprite>(tm, "pu_inv", ANIMATION_FPS));
		AddView<GC_pu_Shock>(Make<Z_Predicate<Z_Const>>(IsPickupVisible, Z_FREE_ITEM),
		                     Make<R_AnimatedSprite>(tm, "pu_shock", ANIMATION_FPS));
		AddView<GC_pu_Shock>(Make<Z_Predicate<Z_Const>>(IsPickupAttached, Z_FREE_ITEM), Make<R_Shock>(tm));
		AddView<GC_pu_Booster>(Make<Z_Predicate<Z_Const>>(And(IsPickupVisible, Not(IsPickupAttached)), Z_FREE_ITEM),
		                       Make<R_AnimatedSprite>(tm, "pu_booster", ANIMATION_FPS));
		AddView<GC_pu_Booster>(Make<Z_Predicate<Z_Const>>(And(IsPickupVisible, IsPickupAttached), Z_FREE_ITEM),
		                       Make<R_Booster>(tm));

		AddView<GC_Wood>(Make<Z_Const>(Z_WOOD), Make<R_Tile>(tm, "wood", 0xffffffff, vec2d{ 0, 0 }, true));
		AddView<GC_Wood>(Make<Z_Const>(Z_SHADOW), Make<R_Tile>(tm, "wood_shadow", 0x50000000, vec2d{ 8, 8 }, false));
		AddView<GC_Water>(Make<Z_Const>(Z_WATER), Make<R_Tile>(tm, "water", 0xffffffff, vec2d{ 0, 0 }, true));

		AddView<GC_BrickFragment>(Make<Z_Const>(Z_PARTICLE), Make<R_BrickFragment>(tm));
		AddView<GC_Particle>(Make<Z_Const>(Z_PARTICLE), Make<R_Particle>(tm));
		AddView<GC_ParticleExplosion>(Make<Z_Const>(Z_EXPLODE), Make<R_Particle>(tm));
		AddView<GC_ParticleDecal>(Make<Z_Const>(Z_WATER), Make<R_Particle>(tm));
		AddView<GC_ParticleGauss>(Make<Z_Const>(Z_GAUSS_RAY), Make<R_Particle>(tm));

		AddView<GC_UserObject>(Make<Z_UserObject>(), Make<R_UserObject>(tm));
		AddView<GC_Decoration>(Make<Z_Decoration>(), Make<R_Decoration>(tm));
	}
};

class EditorViewsBuilder : public ObjectViewsSelectorBuilder
{
public:
	EditorViewsBuilder(TextureManager &tm)
	{
		AddView<GC_SpawnPoint>(Make<Z_Const>(Z_EDITOR), Make<R_Sprite>(tm, "editor_respawn"));
		AddView<GC_Trigger>(Make<Z_Const>(Z_WOOD), Make<R_Sprite>(tm, "editor_trigger"));
	}
};

class NightViewsBuilder : public ObjectViewsSelectorBuilder
{
public:
	NightViewsBuilder(TextureManager &tm)
	{
		AddView<GC_Light>(Make<Z_Light>(), Make<R_Light>(tm));
	}
};

RenderScheme::RenderScheme(TextureManager &tm)
	: _gameViews(GameViewsBuilder(tm))
	, _editorViews(EditorViewsBuilder(tm))
	, _nightViews(NightViewsBuilder(tm))
{
}

ViewCollection RenderScheme::GetViews(const GC_Actor &actor, bool editorMode, bool nightMode) const
{
	// In editor mode give priority to _editorViews
	ViewCollection viewCollection = {};
	if (editorMode)
		viewCollection = _editorViews.GetViews(actor);
	if (begin(viewCollection) == end(viewCollection) && nightMode)
		viewCollection = _nightViews.GetViews(actor);
	if (begin(viewCollection) == end(viewCollection))
		viewCollection = _gameViews.GetViews(actor);
	return viewCollection;
}
