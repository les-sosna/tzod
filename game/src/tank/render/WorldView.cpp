#include "WorldView.h"
#include "RenderScheme.h"

#include "rLight.h"

#include "gc/Light.h"
#include "gc/Macros.h"
#include "gc/World.h"

#include "constants.h" // LOCATION_SIZE
#include "video/RenderBase.h"
#include "config/Config.h"

WorldView::WorldView(IRender &render, TextureManager &tm, RenderScheme &rs)
    : _render(render)
    , _tm(tm)
    , _renderScheme(rs)
	, _terrain(tm)
{
}

WorldView::~WorldView()
{
}

void WorldView::Render(DrawingContext &dc, World &world, const FRECT &view, bool editorMode) const
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
			if( auto *viewCollection = _renderScheme.GetViews(*object, editorMode) )
			{
				for( auto &view: *viewCollection )
				{
					enumZOrder z = view.zfunc->GetZ(world, *object);
					if( Z_NONE != z && object->GetGridSet() )
						zLayers[z].emplace_back(object, view.rfunc.get());
				}
			}
        }
    }

    FOREACH( world.GetList(LIST_gsprites), GC_2dSprite, object )
    {
		if( auto *viewCollection = _renderScheme.GetViews(*object, editorMode) )
		{
			for( auto &view: *viewCollection )
			{
				enumZOrder z = view.zfunc->GetZ(world, *object);
				if( Z_NONE != z && !object->GetGridSet() )
					zLayers[z].emplace_back(object, view.rfunc.get());
			}
		}
    }

    for( int z = 0; z < Z_COUNT; ++z )
    {
        for( auto &actorWithView: zLayers[z] )
			actorWithView.second->Draw(world, *actorWithView.first, dc);
        zLayers[z].clear();
    }
}
