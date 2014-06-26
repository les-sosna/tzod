#include "WorldView.h"
#include "rAnimatedSprite.h"
#include "rLight.h"
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
#include "gc/projectiles.h"
#include "gc/Turrets.h"
#include "gc/Vehicle.h"
#include "gc/Weapons.h"
#include "gc/Macros.h"

#include "constants.h" // TODO: ANIMATION_FPS


WorldView::WorldView(IRender &render, TextureManager &tm)
    : _render(render)
    , _tm(tm)
	, _terrain(tm)
{
	AddView<GC_Wall, R_Wall>(tm, "brick");
	AddView<GC_Wall_Concrete, R_Wall>(tm, "concrete");
	
	AddView<GC_Crate, R_Sprite>(tm, "crate01", Z_WALLS);
	
	AddView<GC_TankBullet, R_Sprite>(tm, "projectile_cannon", Z_PROJECTILE);
	AddView<GC_Rocket, R_Sprite>(tm, "projectile_rocket", Z_PROJECTILE);
//	AddView<GC_Bullet, R_Sprite>(tm, "projectile_bullet", Z_PROJECTILE);
	AddView<GC_PlazmaClod, R_Sprite>(tm, "projectile_plazma", Z_PROJECTILE);
	AddView<GC_BfgCore, R_AnimatedSprite>(tm, "projectile_bfg", Z_PROJECTILE, ANIMATION_FPS);
// TODO:	AddView<GC_FireSpark, xxx>(tm, "projectile_fire", Z_PROJECTILE);
	AddView<GC_ACBullet, R_Sprite>(tm, "projectile_ac", Z_PROJECTILE);
	AddView<GC_Disk, R_Sprite>(tm, "projectile_disk", Z_PROJECTILE);
	
	AddView<GC_Spotlight, R_Sprite>(tm, "spotlight", Z_PROJECTILE);
	
	AddView<GC_Tank_Light, R_Vehicle>(tm);
	
	AddView<GC_Text_ToolTip, R_Text>(tm);
	
	AddView<GC_TurretCannon, R_Turret>(tm, "turret_platform", "turret_cannon");
	AddView<GC_TurretRocket, R_Turret>(tm, "turret_platform", "turret_rocket");
	AddView<GC_TurretMinigun, R_Turret>(tm, "turret_mg_wake", "turret_mg");
	AddView<GC_TurretGauss, R_Turret>(tm, "turret_gauss_wake", "turret_gauss");
	
	AddView<GC_Weap_RocketLauncher, R_Weapon>(tm, "weap_ak47");
	AddView<GC_Weap_AutoCannon, R_Weapon>(tm, "weap_ac");
	AddView<GC_Weap_Cannon, R_Weapon>(tm, "weap_cannon");
	AddView<GC_Weap_Plazma, R_Weapon>(tm, "weap_plazma");
	AddView<GC_Weap_Gauss, R_Weapon>(tm, "weap_gauss");
	AddView<GC_Weap_Ram, R_Weapon>(tm, "weap_ram");
	AddView<GC_Weap_BFG, R_Weapon>(tm, "weap_bfg");
	AddView<GC_Weap_Ripper, R_WeaponRipper>(tm);
	AddView<GC_Weap_Minigun, R_WeaponMinigun>(tm);
	AddView<GC_Weap_Zippo, R_Weapon>(tm, "weap_zippo");

//	AddView<GC_pu_Health, R_AnimatedSprite>(tm, "pu_health", Z_FREE_ITEM, ANIMATION_FPS);
	AddView<GC_pu_Mine, R_Sprite>(tm, "item_mine", Z_FREE_ITEM);
//	AddView<GC_pu_Shield, xxx>(tm);
//	AddView<GC_pu_Shock, xxx>(tm);
//	AddView<GC_pu_Booster, xxx>(tm);

	
	AddView<GC_Wood, R_Tile>(tm, "wood", Z_WOOD);
	AddView<GC_Water, R_Tile>(tm, "water", Z_WATER);
}

WorldView::~WorldView()
{
}

ObjectView* WorldView::GetView(const GC_Actor &actor) const
{
	ObjectType type = actor.GetType();
	return type < _type2view.size() ? _type2view[type].get() : nullptr;
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

    static std::vector<GC_2dSprite*> zLayers[Z_COUNT];
    for( int x = xmin; x <= xmax; ++x )
    for( int y = ymin; y <= ymax; ++y )
    {
        FOREACH(world.grid_sprites.element(x,y), GC_2dSprite, object)
        {
			if( ObjectView *view = GetView(*object) )
			{
				enumZOrder z = view->GetZ(*object);
				if( Z_NONE != z && object->GetGridSet() )
					zLayers[z].push_back(object);
			}
			else
			{
				// TODO: remove fallback to old render
				if( object->GetVisible() && Z_NONE != object->GetZ() && object->GetGridSet() )
					zLayers[object->GetZ()].push_back(object);
			}
        }
    }

    FOREACH( world.GetList(LIST_gsprites), GC_2dSprite, object )
    {
		if( ObjectView *view = GetView(*object) )
		{
			enumZOrder z = view->GetZ(*object);
			if( Z_NONE != z && !object->GetGridSet() )
				zLayers[z].push_back(object);
		}
		else
		{
			// TODO: remove fallback to old render
			if( object->GetVisible() && Z_NONE != object->GetZ() && !object->GetGridSet() )
				zLayers[object->GetZ()].push_back(object);
		}
    }

	DrawingContext &dc = static_cast<DrawingContext&>(_tm);
    for( int z = 0; z < Z_COUNT; ++z )
    {
        for( GC_2dSprite *sprite: zLayers[z] )
		{
			if( ObjectView *view = GetView(*sprite) )
				view->Draw(world, *sprite, dc);
			else
				// TODO: remove fallback to old render
				sprite->Draw(dc, editorMode);
		}
        zLayers[z].clear();
    }
}
