#include "WorldView.h"
#include "rAnimatedSprite.h"
#include "rBrickFragment.h"
#include "rFireSpark.h"
#include "rIndicator.h"
#include "rLight.h"
#include "rMinigun.h"
#include "rParticle.h"
#include "rPredicate.h"
#include "rSprite.h"
#include "rText.h"
#include "rTile.h"
#include "rTurret.h"
#include "rVehicle.h"
#include "rWall.h"
#include "rWeapon.h"
#include "rWeaponBase.h"
#include "config/Config.h"
#include "gc/Camera.h"
#include "gc/Light.h"
#include "gc/World.h"
#include "video/RenderBase.h"

#include "gc/RigidBody.h"
#include "gc/Crate.h"
#include "gc/GameClasses.h"
#include "gc/indicators.h"
#include "gc/particles.h"
#include "gc/projectiles.h"
#include "gc/Trigger.h"
#include "gc/Turrets.h"
#include "gc/Vehicle.h"
#include "gc/Weapons.h"
#include "gc/Macros.h"

#include "constants.h" // TODO: ANIMATION_FPS



static bool IsWeaponAdvanced(const World &world, const GC_Actor &actor)
{
	assert(dynamic_cast<const GC_Weapon*>(&actor));
	return static_cast<const GC_Weapon&>(actor).GetAdvanced();
}
static bool IsPickupVisible(const World &world, const GC_Actor &actor)
{
	assert(dynamic_cast<const GC_Pickup*>(&actor));
	return static_cast<const GC_Pickup&>(actor).GetVisible();
}
static bool IsPickupAttached(const World &world, const GC_Actor &actor)
{
	assert(dynamic_cast<const GC_Pickup*>(&actor));
	return nullptr != static_cast<const GC_Pickup&>(actor).GetCarrier();
}

template <class T, class ...Args>
static std::unique_ptr<T> Make(Args && ...args)
{
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}


WorldView::WorldView(IRender &render, TextureManager &tm)
    : _render(render)
    , _tm(tm)
	, _terrain(tm)
{
	_gameViews.AddView<GC_Wall>(Make<Z_Const>(Z_WALLS), Make<R_Wall>(tm, "brick"));
	_gameViews.AddView<GC_Wall_Concrete>(Make<Z_Const>(Z_WALLS), Make<R_Wall>(tm, "concrete"));
	
	_gameViews.AddView<GC_Crate>(Make<Z_Const>(Z_WALLS), Make<R_Sprite>(tm, "crate01"));
	
	_gameViews.AddView<GC_TankBullet>(Make<Z_Const>(Z_PROJECTILE), Make<R_Sprite>(tm, "projectile_cannon"));
	_gameViews.AddView<GC_Rocket>(Make<Z_Const>(Z_PROJECTILE), Make<R_Sprite>(tm, "projectile_rocket"));
//	_gameViews.AddView<GC_Bullet>(Make<Z_Const>(Z_PROJECTILE), Make<R_Sprite>(tm, "projectile_bullet"));
	_gameViews.AddView<GC_PlazmaClod>(Make<Z_Const>(Z_PROJECTILE), Make<R_Sprite>(tm, "projectile_plazma"));
	_gameViews.AddView<GC_BfgCore>(Make<Z_Const>(Z_PROJECTILE), Make<R_AnimatedSprite>(tm, "projectile_bfg", 10));
	_gameViews.AddView<GC_FireSpark>(Make<Z_Const>(Z_PROJECTILE), Make<R_FireSpark>(tm));
	_gameViews.AddView<GC_ACBullet>(Make<Z_Const>(Z_PROJECTILE), Make<R_Sprite>(tm, "projectile_ac"));
	_gameViews.AddView<GC_Disk>(Make<Z_Const>(Z_PROJECTILE), Make<R_Sprite>(tm, "projectile_disk"));
	
	_gameViews.AddView<GC_Spotlight>(Make<Z_Const>(Z_PROJECTILE), Make<R_Sprite>(tm, "spotlight"));
	_gameViews.AddView<GC_Light>(Make<Z_Light>(), Make<R_Light>(tm));
	
	_gameViews.AddView<GC_Tank_Light>(Make<Z_Const>(Z_VEHICLES), Make<R_Vehicle>(tm));
	_gameViews.AddView<GC_Tank_Light>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_HealthIndicator>(tm));
	
	_gameViews.AddView<GC_Text_ToolTip>(Make<Z_Const>(Z_PARTICLE), Make<R_Text>(tm));
	
	_gameViews.AddView<GC_TurretCannon>(Make<Z_Const>(Z_WALLS), Make<R_Turret>(tm, "turret_platform", "turret_cannon"));
	_gameViews.AddView<GC_TurretCannon>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_HealthIndicator>(tm));
	_gameViews.AddView<GC_TurretRocket>(Make<Z_Const>(Z_WALLS), Make<R_Turret>(tm, "turret_platform", "turret_rocket"));
	_gameViews.AddView<GC_TurretRocket>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_HealthIndicator>(tm));
	_gameViews.AddView<GC_TurretMinigun>(Make<Z_Const>(Z_WALLS), Make<R_Turret>(tm, "turret_mg_wake", "turret_mg"));
	_gameViews.AddView<GC_TurretMinigun>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_HealthIndicator>(tm));
	_gameViews.AddView<GC_TurretGauss>(Make<Z_Const>(Z_WALLS), Make<R_Turret>(tm, "turret_gauss_wake", "turret_gauss"));
	_gameViews.AddView<GC_TurretGauss>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_HealthIndicator>(tm));
	
	_gameViews.AddView<GC_Weap_RocketLauncher>(Make<Z_Weapon>(), Make<R_Weapon>(tm, "weap_ak47"));
	_gameViews.AddView<GC_Weap_RocketLauncher>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_Crosshair>(tm));
	_gameViews.AddView<GC_Weap_RocketLauncher>(Make<Z_WeapFireEffect>(0.1f), Make<R_WeapFireEffect>(tm, "particle_fire3", 0.1f, 13.0f, true));
	_gameViews.AddView<GC_Weap_AutoCannon>(Make<Z_Weapon>(), Make<R_Weapon>(tm, "weap_ac"));
	_gameViews.AddView<GC_Weap_AutoCannon>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_Crosshair>(tm));
	_gameViews.AddView<GC_Weap_AutoCannon>(Make<Z_Predicate<Z_WeapFireEffect>>(IsWeaponAdvanced, 0.135f), Make<R_WeapFireEffect>(tm, "particle_fire4", 0.135f, 17.0f, true));
	_gameViews.AddView<GC_Weap_AutoCannon>(Make<Z_Predicate<Z_WeapFireEffect>>(Not(IsWeaponAdvanced), 0.135f), Make<R_WeapFireEffect>(tm, "particle_fire3", 0.135f, 17.0f, true));
	_gameViews.AddView<GC_Weap_AutoCannon>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_AmmoIndicator>(tm));
	_gameViews.AddView<GC_Weap_Cannon>(Make<Z_Weapon>(), Make<R_Weapon>(tm, "weap_cannon"));
	_gameViews.AddView<GC_Weap_Cannon>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_Crosshair>(tm));
	_gameViews.AddView<GC_Weap_Cannon>(Make<Z_WeapFireEffect>(0.2f), Make<R_WeapFireEffect>(tm, "particle_fire3", 0.2f, 21.0f, true));
	_gameViews.AddView<GC_Weap_Plazma>(Make<Z_Weapon>(), Make<R_Weapon>(tm, "weap_plazma"));
	_gameViews.AddView<GC_Weap_Plazma>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_Crosshair>(tm));
	_gameViews.AddView<GC_Weap_Plazma>(Make<Z_WeapFireEffect>(0.2f), Make<R_WeapFireEffect>(tm, "particle_plazma_fire", 0.2f, 0.0f, true));
	_gameViews.AddView<GC_Weap_Gauss>(Make<Z_Weapon>(), Make<R_Weapon>(tm, "weap_gauss"));
	_gameViews.AddView<GC_Weap_Gauss>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_Crosshair>(tm));
	_gameViews.AddView<GC_Weap_Gauss>(Make<Z_WeapFireEffect>(0.15f), Make<R_WeapFireEffect>(tm, "particle_gaussfire", 0.15f, 0.0f, true));
	_gameViews.AddView<GC_Weap_Ram>(Make<Z_Weapon>(), Make<R_Weapon>(tm, "weap_ram"));
	_gameViews.AddView<GC_Weap_Ram>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_Crosshair>(tm));
	_gameViews.AddView<GC_Weap_Ram>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_FuelIndicator>(tm));
	_gameViews.AddView<GC_Weap_BFG>(Make<Z_Weapon>(), Make<R_Weapon>(tm, "weap_bfg"));
	_gameViews.AddView<GC_Weap_BFG>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_Crosshair>(tm));
	_gameViews.AddView<GC_Weap_Ripper>(Make<Z_Weapon>(), Make<R_Weapon>(tm, "weap_ripper"));
	_gameViews.AddView<GC_Weap_Ripper>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_Crosshair>(tm));
	_gameViews.AddView<GC_Weap_Ripper>(Make<Z_Const>(Z_PROJECTILE), Make<R_RipperDisk>(tm));
	_gameViews.AddView<GC_Weap_Minigun>(Make<Z_Weapon>(), Make<R_WeaponMinigun>(tm));
	_gameViews.AddView<GC_Weap_Minigun>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_Crosshair2>(tm));
	_gameViews.AddView<GC_Weap_Minigun>(Make<Z_Predicate<Z_WeapFireEffect>>(IsWeaponAdvanced, 0.1f), Make<R_WeapFireEffect>(tm, "particle_fire3", 0.1f, 17.0f, true));
	_gameViews.AddView<GC_Weap_Minigun>(Make<Z_WeapFireEffect>(0.1f), Make<R_WeapFireEffect>(tm, "minigun_fire", 0.1f, 20.0f, false));
	_gameViews.AddView<GC_Weap_Zippo>(Make<Z_Weapon>(), Make<R_Weapon>(tm, "weap_zippo"));
	_gameViews.AddView<GC_Weap_Zippo>(Make<Z_Const>(Z_VEHICLE_LABEL), Make<R_Crosshair>(tm));

	_gameViews.AddView<GC_pu_Health>(Make<Z_Predicate<Z_Const>>(IsPickupVisible, Z_FREE_ITEM),
									 Make<R_AnimatedSprite>(tm, "pu_health", ANIMATION_FPS));
	_gameViews.AddView<GC_pu_Mine>(Make<Z_Const>(Z_FREE_ITEM), Make<R_Sprite>(tm, "item_mine"));
	_gameViews.AddView<GC_pu_Shield>(Make<Z_Predicate<Z_Const>>(And(IsPickupVisible, IsPickupAttached), Z_PARTICLE),
									 Make<R_AnimatedSprite>(tm, "shield", ANIMATION_FPS));
	_gameViews.AddView<GC_pu_Shield>(Make<Z_Predicate<Z_Const>>(And(IsPickupVisible, Not(IsPickupAttached)), Z_FREE_ITEM),
									 Make<R_AnimatedSprite>(tm, "pu_inv", ANIMATION_FPS));
	_gameViews.AddView<GC_pu_Shock>(Make<Z_Predicate<Z_Const>>(IsPickupVisible, Z_FREE_ITEM),
									Make<R_AnimatedSprite>(tm, "pu_shock", ANIMATION_FPS));
	_gameViews.AddView<GC_pu_Booster>(Make<Z_Predicate<Z_Const>>(And(IsPickupVisible, Not(IsPickupAttached)), Z_FREE_ITEM),
									  Make<R_AnimatedSprite>(tm, "pu_booster", ANIMATION_FPS));
	_gameViews.AddView<GC_pu_Booster>(Make<Z_Predicate<Z_Const>>(And(IsPickupVisible, IsPickupAttached), Z_FREE_ITEM),
									  Make<R_Sprite>(tm, "booster"));
	
	_gameViews.AddView<GC_Wood>(Make<Z_Const>(Z_WOOD), Make<R_Tile>(tm, "wood"));
	_gameViews.AddView<GC_Water>(Make<Z_Const>(Z_WATER), Make<R_Tile>(tm, "water"));
	
	_gameViews.AddView<GC_BrickFragment>(Make<Z_Const>(Z_PARTICLE), Make<R_BrickFragment>(tm));
	_gameViews.AddView<GC_Particle>(Make<Z_Const>(Z_PARTICLE), Make<R_Particle>(tm));
	_gameViews.AddView<GC_ParticleExplosion>(Make<Z_Const>(Z_EXPLODE), Make<R_Particle>(tm));
	_gameViews.AddView<GC_ParticleDecal>(Make<Z_Const>(Z_WATER), Make<R_Particle>(tm));
	_gameViews.AddView<GC_ParticleGauss>(Make<Z_Const>(Z_GAUSS_RAY), Make<R_Particle>(tm));

	_editorViews.AddView<GC_HideLabel>(Make<Z_Const>(Z_EDITOR), Make<R_Sprite>(tm, "editor_item"));
	_editorViews.AddView<GC_SpawnPoint>(Make<Z_Const>(Z_EDITOR), Make<R_Sprite>(tm, "editor_respawn"));
	_editorViews.AddView<GC_Trigger>(Make<Z_Const>(Z_WOOD), Make<R_Sprite>(tm, "editor_trigger"));
}

WorldView::~WorldView()
{
}

void WorldView::Render(World &world, const FRECT &view, bool editorMode) const
{
    // FIXME: ambient will take effect starting next frame
	_render.SetAmbient(g_conf.sv_nightmode.Get() ? (editorMode ? 0.5f : 0) : 1);

	//
	// draw lights to alpha channel
	//

	_render.SetMode(RM_LIGHT);
	if( g_conf.sv_nightmode.Get() )
	{
		float xmin = std::max(0.0f, view.left);
		float ymin = std::max(0.0f, view.top);
		float xmax = std::min(world._sx, view.right);
		float ymax = std::min(world._sy, view.bottom);

		FOREACH( world.GetList(LIST_lights), GC_Light, pLight )
		{
			if( pLight->GetActive() &&
				pLight->GetPos().x + pLight->GetRenderRadius() > xmin &&
				pLight->GetPos().x - pLight->GetRenderRadius() < xmax &&
				pLight->GetPos().y + pLight->GetRenderRadius() > ymin &&
				pLight->GetPos().y - pLight->GetRenderRadius() < ymax )
			{
			//	_FpsCounter::Inst()->OneMoreLight();
				DrawLight(_render, *pLight);
			}
		}
	}


	//
	// draw world to rgb
	//

	_render.SetMode(RM_WORLD);

	_terrain.Draw(_render, world._sx, world._sy, editorMode);

	int xmin = std::max(0, int(view.left / LOCATION_SIZE));
	int ymin = std::max(0, int(view.top / LOCATION_SIZE));
	int xmax = std::min(world._locationsX - 1, int(view.right / LOCATION_SIZE));
	int ymax = std::min(world._locationsY - 1, int(view.bottom / LOCATION_SIZE) + 1);

    static std::vector<std::pair<const GC_2dSprite*, const ObjectRFunc*>> zLayers[Z_COUNT];
    for( int x = xmin; x <= xmax; ++x )
    for( int y = ymin; y <= ymax; ++y )
    {
        FOREACH(world.grid_sprites.element(x,y), GC_2dSprite, object)
        {
			if( auto *viewCollection = GetViews(*object, editorMode) )
			{
				for( auto &view: *viewCollection )
				{
					enumZOrder z = view.zfunc->GetZ(world, *object);
					if( Z_NONE != z && object->GetGridSet() )
						zLayers[z].emplace_back(object, view.rfunc.get());
				}
			}
			else
			{
				// TODO: remove fallback to old render
				if( object->GetVisible() && Z_NONE != object->GetZ() && object->GetGridSet() )
					zLayers[object->GetZ()].emplace_back(object, nullptr);
			}
        }
    }

    FOREACH( world.GetList(LIST_gsprites), GC_2dSprite, object )
    {
		if( auto *viewCollection = GetViews(*object, editorMode) )
		{
			for( auto &view: *viewCollection )
			{
				enumZOrder z = view.zfunc->GetZ(world, *object);
				if( Z_NONE != z && !object->GetGridSet() )
					zLayers[z].emplace_back(object, view.rfunc.get());
			}
		}
		else
		{
			// TODO: remove fallback to old render
			if( object->GetVisible() && Z_NONE != object->GetZ() && !object->GetGridSet() )
				zLayers[object->GetZ()].emplace_back(object, nullptr);
		}
    }

	DrawingContext &dc = static_cast<DrawingContext&>(_tm);
    for( int z = 0; z < Z_COUNT; ++z )
    {
        for( auto &actorWithView: zLayers[z] )
		{
			if( actorWithView.second )
				actorWithView.second->Draw(world, *actorWithView.first, dc);
			else
				// TODO: remove fallback to old render
				actorWithView.first->Draw(dc, editorMode);
		}
        zLayers[z].clear();
    }
}

inline const ObjectViewsSelector::ViewCollection* WorldView::GetViews(const GC_Actor &actor, bool editorMode) const
{
	// In editor mode give priority to _editorViews
	const ObjectViewsSelector::ViewCollection *viewCollection;
	viewCollection = editorMode ? _editorViews.GetViews(actor) : nullptr;
	viewCollection = viewCollection ? viewCollection : _gameViews.GetViews(actor);
	return viewCollection;
}
