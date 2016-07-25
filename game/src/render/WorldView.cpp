#include "inc/render/WorldView.h"
#include "inc/render/RenderScheme.h"

#include <gc/Light.h>
#include <gc/Macros.h>
#include <gc/World.h>
#include <gc/WorldCfg.h>

#include <video/DrawingContext.h>

WorldView::WorldView(TextureManager &tm, RenderScheme &rs)
    : _renderScheme(rs)
	, _terrain(tm)
{
}

WorldView::~WorldView()
{
}

void WorldView::Render(DrawingContext &dc,
					   const World &world,
					   const RectRB &viewport,
					   vec2d eye,
					   float zoom,
					   bool editorMode,
					   bool drawGrid,
					   bool nightMode) const
{
	eye.x = floor(eye.x * zoom) / zoom;
	eye.y = floor(eye.y * zoom) / zoom;

	dc.Camera(viewport, eye.x, eye.y, zoom);

	float left = floor((eye.x - (float) WIDTH(viewport) / 2 / zoom) * zoom) / zoom;
	float top = floor((eye.y - (float)HEIGHT(viewport) / 2 / zoom) * zoom) / zoom;
	float right = left + (float)WIDTH(viewport) / zoom;
	float bottom = top + (float)HEIGHT(viewport) / zoom;

	//
	// draw lights to alpha channel
	//

	dc.SetAmbient(nightMode ? (editorMode ? 0.5f : 0) : 1);
	dc.SetMode(RM_LIGHT); // this will clear the render target with the ambient set above
	if( nightMode )
	{
		float xmin = std::max(world._bounds.left, left);
		float ymin = std::max(world._bounds.top, top);
		float xmax = std::min(world._bounds.right, right);
		float ymax = std::min(world._bounds.bottom, bottom);

		FOREACH( world.GetList(LIST_lights), const GC_Light, pLight )
		{
			if( pLight->GetActive() &&
				pLight->GetPos().x + pLight->GetRenderRadius() > xmin &&
				pLight->GetPos().x - pLight->GetRenderRadius() < xmax &&
				pLight->GetPos().y + pLight->GetRenderRadius() > ymin &&
				pLight->GetPos().y - pLight->GetRenderRadius() < ymax )
			{
				float intensity = pLight->GetIntensity();
				if (pLight->GetFade())
				{
					float age = (world.GetTime() - pLight->GetStartTime()) / pLight->GetTimeout();
					intensity *= 1.0f - age*age*age;
				}
				switch (pLight->GetLightType())
				{
					case GC_Light::LIGHT_POINT:
						dc.DrawPointLight(intensity, pLight->GetRadius(), pLight->GetPos());
						break;
					case GC_Light::LIGHT_SPOT:
						dc.DrawSpotLight(intensity, pLight->GetRadius(), pLight->GetPos(),
										 pLight->GetLightDirection(), pLight->GetOffset(), pLight->GetAspect());
						break;
					case GC_Light::LIGHT_DIRECT:
						dc.DrawDirectLight(intensity, pLight->GetRadius(), pLight->GetPos(),
										   pLight->GetLightDirection(), pLight->GetLength());
						break;
					default:
						assert(false);
				}
			}
		}
	}


	static std::vector<std::pair<const GC_Actor*, const ObjectRFunc*>> zLayers[Z_COUNT];

	int xmin = std::max(world._locationBounds.left, (int)std::floor(left / LOCATION_SIZE));
	int ymin = std::max(world._locationBounds.top, (int)std::floor(top / LOCATION_SIZE));
	int xmax = std::min(world._locationBounds.right - 1, (int)std::floor(right / LOCATION_SIZE));
	int ymax = std::min(world._locationBounds.bottom - 1, (int)std::floor(bottom / LOCATION_SIZE) + 1);
	for( int x = xmin; x <= xmax; ++x )
	for( int y = ymin; y <= ymax; ++y )
	{
		FOREACH(world.grid_actors.element(x, y), const GC_Actor, object)
		{
			if( auto *viewCollection = _renderScheme.GetViews(*object, editorMode, nightMode) )
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

	FOREACH( world.GetList(LIST_gsprites), GC_Actor, object )
	{
		if( auto *viewCollection = _renderScheme.GetViews(*object, editorMode, nightMode) )
		{
			for( auto &view: *viewCollection )
			{
				enumZOrder z = view.zfunc->GetZ(world, *object);
				if( Z_NONE != z && !object->GetGridSet() )
					zLayers[z].emplace_back(object, view.rfunc.get());
			}
		}
	}


	//
	// draw world to rgb
	//

	dc.SetMode(RM_WORLD);

	_terrain.Draw(dc, world._bounds, drawGrid);

	for( int z = 0; z < Z_COUNT; ++z )
	{
		for( auto &actorWithView: zLayers[z] )
			actorWithView.second->Draw(world, *actorWithView.first, dc);
		zLayers[z].clear();
	}

	dc.SetMode(RM_INTERFACE);
}
