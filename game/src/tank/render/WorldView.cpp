#include "WorldView.h"
#include "RenderScheme.h"

#include "constants.h" // LOCATION_SIZE

#include "gc/Light.h"
#include "gc/Macros.h"
#include "gc/World.h"

#include "video/DrawingContext.h"
#include "config/Config.h"

WorldView::WorldView(TextureManager &tm, RenderScheme &rs)
    : _renderScheme(rs)
	, _terrain(tm)
{
}

WorldView::~WorldView()
{
}

void WorldView::Render(DrawingContext &dc, World &world, const Rect &viewport, const vec2d &eye, float zoom, bool editorMode) const
{
	dc.Camera(viewport, eye.x, eye.y, zoom);

	float left = floor((eye.x - (float) WIDTH(viewport) / 2 / zoom) * zoom) / zoom;
	float top = floor((eye.y - (float)HEIGHT(viewport) / 2 / zoom) * zoom) / zoom;
	float right = left + (float)WIDTH(viewport) / zoom;
	float bottom = top + (float)HEIGHT(viewport) / zoom;

	//
	// draw lights to alpha channel
	//

	dc.SetAmbient(g_conf.sv_nightmode.Get() ? (editorMode ? 0.5f : 0) : 1);
	dc.SetMode(RM_LIGHT); // this will clear the render target with the ambient set above
	if( g_conf.sv_nightmode.Get() )
	{
		float xmin = std::max(0.0f, left);
		float ymin = std::max(0.0f, top);
		float xmax = std::min(world._sx, right);
		float ymax = std::min(world._sy, bottom);

		FOREACH( world.GetList(LIST_lights), GC_Light, pLight )
		{
			if( pLight->GetActive() &&
				pLight->GetPos().x + pLight->GetRenderRadius() > xmin &&
				pLight->GetPos().x - pLight->GetRenderRadius() < xmax &&
				pLight->GetPos().y + pLight->GetRenderRadius() > ymin &&
				pLight->GetPos().y - pLight->GetRenderRadius() < ymax )
			{
			//	_FpsCounter::Inst()->OneMoreLight();
				switch (pLight->GetLightType())
				{
					case GC_Light::LIGHT_POINT:
						dc.DrawPointLight(pLight->GetIntensity(), pLight->GetRadius(), pLight->GetPos());
						break;
					case GC_Light::LIGHT_SPOT:
						dc.DrawSpotLight(pLight->GetIntensity(), pLight->GetRadius(), pLight->GetPos(),
										 pLight->GetLightDirection(), pLight->GetOffset(), pLight->GetAspect());
						break;
					case GC_Light::LIGHT_DIRECT:
						dc.DrawDirectLight(pLight->GetIntensity(), pLight->GetRadius(), pLight->GetPos(),
										   pLight->GetLightDirection(), pLight->GetLength());
						break;
					default:
						assert(false);
				}
			}
		}
	}


	//
	// draw world to rgb
	//

	dc.SetMode(RM_WORLD);

	_terrain.Draw(dc, world._sx, world._sy, editorMode);

	int xmin = std::max(0, int(left / LOCATION_SIZE));
	int ymin = std::max(0, int(top / LOCATION_SIZE));
	int xmax = std::min(world._locationsX - 1, int(right / LOCATION_SIZE));
	int ymax = std::min(world._locationsY - 1, int(bottom / LOCATION_SIZE) + 1);

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
