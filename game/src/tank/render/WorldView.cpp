#include "WorldView.h"
#include "rAnimatedSprite.h"
#include "rIndicator.h"
#include "rLight.h"
#include "rMinigun.h"
#include "rPredicate.h"
#include "rSprite.h"
#include "rText.h"
#include "rTile.h"
#include "rTurret.h"
#include "rVehicle.h"
#include "rWall.h"
#include "rWeapon.h"
#include "config/Config.h"
#include "gc/Camera.h"
#include "gc/Light.h"
#include "gc/World.h"
#include "video/RenderBase.h"

#include "gc/RigidBody.h"
#include "gc/Crate.h"
#include "gc/GameClasses.h"
#include "gc/Indicators.h"
#include "gc/projectiles.h"
#include "gc/Trigger.h"
#include "gc/Turrets.h"
#include "gc/Vehicle.h"
#include "gc/Weapons.h"
#include "gc/Macros.h"

#include "constants.h" // TODO: ANIMATION_FPS


const ObjectViewsSelector::ViewCollection* ObjectViewsSelector::GetViews(const GC_Actor &actor) const
{
	ObjectType type = actor.GetType();
	return (type < _type2views.size() && !_type2views[type].empty()) ? &_type2views[type] : nullptr;
}


static bool IsWeaponAdvanced(const World &world, const GC_Actor &actor)
{
	assert(dynamic_cast<const GC_Weapon&>(actor));
	return static_cast<const GC_Weapon&>(actor).GetAdvanced();
}
static bool IsWeaponNormal(const World &world, const GC_Actor &actor)
{
	assert(dynamic_cast<const GC_Weapon&>(actor));
	return !static_cast<const GC_Weapon&>(actor).GetAdvanced();
}


WorldView::WorldView(IRender &render, TextureManager &tm)
    : _render(render)
    , _tm(tm)
	, _terrain(tm)
{
	_gameViews.AddView<GC_Wall, R_Wall>(tm, "brick");
	_gameViews.AddView<GC_Wall_Concrete, R_Wall>(tm, "concrete");
	
	_gameViews.AddView<GC_Crate, R_Sprite>(tm, "crate01", Z_WALLS);
	
	_gameViews.AddView<GC_TankBullet, R_Sprite>(tm, "projectile_cannon", Z_PROJECTILE);
	_gameViews.AddView<GC_Rocket, R_Sprite>(tm, "projectile_rocket", Z_PROJECTILE);
//	_gameViews.AddView<GC_Bullet, R_Sprite>(tm, "projectile_bullet", Z_PROJECTILE);
	_gameViews.AddView<GC_PlazmaClod, R_Sprite>(tm, "projectile_plazma", Z_PROJECTILE);
	_gameViews.AddView<GC_BfgCore, R_AnimatedSprite>(tm, "projectile_bfg", Z_PROJECTILE, 10);
// TODO:	_gameViews.AddView<GC_FireSpark, xxx>(tm, "projectile_fire", Z_PROJECTILE);
	_gameViews.AddView<GC_ACBullet, R_Sprite>(tm, "projectile_ac", Z_PROJECTILE);
	_gameViews.AddView<GC_Disk, R_Sprite>(tm, "projectile_disk", Z_PROJECTILE);
	
	_gameViews.AddView<GC_Spotlight, R_Sprite>(tm, "spotlight", Z_PROJECTILE);
	_gameViews.AddView<GC_Light, R_Light>(tm);
	
	_gameViews.AddView<GC_Tank_Light, R_Vehicle>(tm);
	_gameViews.AddView<GC_Tank_Light, R_HealthIndicator>(tm);
	
	_gameViews.AddView<GC_Text_ToolTip, R_Text>(tm);
	
	_gameViews.AddView<GC_TurretCannon, R_Turret>(tm, "turret_platform", "turret_cannon");
	_gameViews.AddView<GC_TurretCannon, R_HealthIndicator>(tm);
	_gameViews.AddView<GC_TurretRocket, R_Turret>(tm, "turret_platform", "turret_rocket");
	_gameViews.AddView<GC_TurretRocket, R_HealthIndicator>(tm);
	_gameViews.AddView<GC_TurretMinigun, R_Turret>(tm, "turret_mg_wake", "turret_mg");
	_gameViews.AddView<GC_TurretMinigun, R_HealthIndicator>(tm);
	_gameViews.AddView<GC_TurretGauss, R_Turret>(tm, "turret_gauss_wake", "turret_gauss");
	_gameViews.AddView<GC_TurretGauss, R_HealthIndicator>(tm);
	
	_gameViews.AddView<GC_Weap_RocketLauncher, R_Weapon>(tm, "weap_ak47");
	_gameViews.AddView<GC_Weap_RocketLauncher, R_Crosshair>(tm);
	_gameViews.AddView<GC_Weap_RocketLauncher, R_WeapFireEffect>(tm, "particle_fire3", 0.1f, 13.0f, true);
	_gameViews.AddView<GC_Weap_AutoCannon, R_Weapon>(tm, "weap_ac");
	_gameViews.AddView<GC_Weap_AutoCannon, R_Crosshair>(tm);
	_gameViews.AddView<GC_Weap_AutoCannon, R_Predicate<R_WeapFireEffect>>(IsWeaponAdvanced, tm, "particle_fire4", 0.135f, 17.0f, true);
	_gameViews.AddView<GC_Weap_AutoCannon, R_Predicate<R_WeapFireEffect>>(IsWeaponNormal, tm, "particle_fire3", 0.135f, 17.0f, true);
	_gameViews.AddView<GC_Weap_AutoCannon, R_AmmoIndicator>(tm);
	_gameViews.AddView<GC_Weap_Cannon, R_Weapon>(tm, "weap_cannon");
	_gameViews.AddView<GC_Weap_Cannon, R_Crosshair>(tm);
	_gameViews.AddView<GC_Weap_Cannon, R_WeapFireEffect>(tm, "particle_fire3", 0.2f, 21.0f, true);
	_gameViews.AddView<GC_Weap_Plazma, R_Weapon>(tm, "weap_plazma");
	_gameViews.AddView<GC_Weap_Plazma, R_Crosshair>(tm);
	_gameViews.AddView<GC_Weap_Plazma, R_WeapFireEffect>(tm, "particle_plazma_fire", 0.2f, 0.0f, true);
	_gameViews.AddView<GC_Weap_Gauss, R_Weapon>(tm, "weap_gauss");
	_gameViews.AddView<GC_Weap_Gauss, R_Crosshair>(tm);
	_gameViews.AddView<GC_Weap_Gauss, R_WeapFireEffect>(tm, "particle_gaussfire", 0.15f, 0.0f, true);
	_gameViews.AddView<GC_Weap_Ram, R_Weapon>(tm, "weap_ram");
	_gameViews.AddView<GC_Weap_Ram, R_Crosshair>(tm);
	_gameViews.AddView<GC_Weap_Ram, R_FuelIndicator>(tm);
	_gameViews.AddView<GC_Weap_BFG, R_Weapon>(tm, "weap_bfg");
	_gameViews.AddView<GC_Weap_BFG, R_Crosshair>(tm);
	_gameViews.AddView<GC_Weap_Ripper, R_Weapon>(tm, "weap_ripper");
	_gameViews.AddView<GC_Weap_Ripper, R_Crosshair>(tm);
	_gameViews.AddView<GC_Weap_Ripper, R_RipperDisk>(tm);
	_gameViews.AddView<GC_Weap_Minigun, R_WeaponMinigun>(tm);
	_gameViews.AddView<GC_Weap_Minigun, R_Crosshair2>(tm);
	_gameViews.AddView<GC_Weap_Minigun, R_Predicate<R_WeapFireEffect>>(IsWeaponAdvanced, tm, "particle_fire3", 0.1f, 17.0f, true);
	_gameViews.AddView<GC_Weap_Minigun, R_WeapFireEffect>(tm, "minigun_fire", 0.1f, 20.0f, false);
	_gameViews.AddView<GC_Weap_Zippo, R_Weapon>(tm, "weap_zippo");
	_gameViews.AddView<GC_Weap_Zippo, R_Crosshair>(tm);

//	_gameViews.AddView<GC_pu_Health, R_AnimatedSprite>(tm, "pu_health", Z_FREE_ITEM, ANIMATION_FPS);
	_gameViews.AddView<GC_pu_Mine, R_Sprite>(tm, "item_mine", Z_FREE_ITEM);
//	_gameViews.AddView<GC_pu_Shield, xxx>(tm);
//	_gameViews.AddView<GC_pu_Shock, xxx>(tm);
//	_gameViews.AddView<GC_pu_Booster, xxx>(tm);
	
	_gameViews.AddView<GC_Wood, R_Tile>(tm, "wood", Z_WOOD);
	_gameViews.AddView<GC_Water, R_Tile>(tm, "water", Z_WATER);

	_editorViews.AddView<GC_HideLabel, R_Sprite>(tm, "editor_item", Z_EDITOR);
	_editorViews.AddView<GC_SpawnPoint, R_Sprite>(tm, "editor_respawn", Z_EDITOR);
	_editorViews.AddView<GC_Trigger, R_Sprite>(tm, "editor_trigger", Z_WOOD);
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

    static std::vector<std::pair<const GC_2dSprite*, const ObjectView*>> zLayers[Z_COUNT];
    for( int x = xmin; x <= xmax; ++x )
    for( int y = ymin; y <= ymax; ++y )
    {
        FOREACH(world.grid_sprites.element(x,y), GC_2dSprite, object)
        {
			if( auto *viewCollection = GetViews(*object, editorMode) )
			{
				for( auto &view: *viewCollection )
				{
					enumZOrder z = view->GetZ(world, *object);
					if( Z_NONE != z && object->GetGridSet() )
						zLayers[z].emplace_back(object, view.get());
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
				enumZOrder z = view->GetZ(world, *object);
				if( Z_NONE != z && !object->GetGridSet() )
					zLayers[z].emplace_back(object, view.get());
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
