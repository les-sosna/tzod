#include "inc/render/WorldView.h"
#include "inc/render/RenderScheme.h"
#include <ai/ai.h>
#include <ctx/AIManager.h>
#include <gc/Field.h>
#include <gc/Light.h>
#include <gc/Macros.h>
#include <gc/World.h>
#include <gc/WorldCfg.h>

#include <video/RenderContext.h>
#include <video/TextureManager.h>

WorldView::WorldView(TextureManager &tm, RenderScheme &rs)
	: _renderScheme(rs)
	, _terrain(tm)
	, _lineTex(tm.FindSprite("dotted_line"))
	, _texField(tm.FindSprite("ui/selection"))
{
}

WorldView::~WorldView()
{
}

void WorldView::Render(RenderContext &rc,
                       const World &world,
                       WorldViewRenderOptions options,
                       const AIManager *aiManager) const
{
	FRECT visibleRegion = rc.GetVisibleRegion();

	// draw lights to alpha channel
	rc.SetAmbient(options.nightMode ? (options.editorMode ? 0.5f : 0) : 1);
	rc.SetMode(RM_LIGHT); // this will clear the render target with the ambient set above
	if( options.nightMode )
	{
		float xmin = std::max(world.GetBounds().left, visibleRegion.left);
		float ymin = std::max(world.GetBounds().top, visibleRegion.top);
		float xmax = std::min(world.GetBounds().right, visibleRegion.right);
		float ymax = std::min(world.GetBounds().bottom, visibleRegion.bottom);

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
						rc.DrawPointLight(intensity, pLight->GetRadius(), pLight->GetPos());
						break;
					case GC_Light::LIGHT_SPOT:
						rc.DrawSpotLight(intensity, pLight->GetRadius(), pLight->GetPos(),
						                 pLight->GetLightDirection(), pLight->GetOffset(), pLight->GetAspect());
						break;
					case GC_Light::LIGHT_DIRECT:
						rc.DrawDirectLight(intensity, pLight->GetRadius(), pLight->GetPos(),
						                   pLight->GetLightDirection(), pLight->GetLength());
						break;
					default:
						assert(false);
				}
			}
		}
	}


	static std::vector<std::pair<const GC_MovingObject*, const ObjectRFunc*>> zLayers[Z_COUNT];

	int xmin = std::max(world.GetLocationBounds().left, (int)std::floor(visibleRegion.left / WORLD_LOCATION_SIZE - 0.5f));
	int ymin = std::max(world.GetLocationBounds().top, (int)std::floor(visibleRegion.top / WORLD_LOCATION_SIZE - 0.5f));
	int xmax = std::min(world.GetLocationBounds().right - 1, (int)std::floor(visibleRegion.right / WORLD_LOCATION_SIZE + 0.5f));
	int ymax = std::min(world.GetLocationBounds().bottom - 1, (int)std::floor(visibleRegion.bottom / WORLD_LOCATION_SIZE + 0.5f));
	for( int x = xmin; x <= xmax; ++x )
	for( int y = ymin; y <= ymax; ++y )
	{
		FOREACH(world.grid_moving.element(x, y), const GC_MovingObject, object)
		{
			for( auto &view: _renderScheme.GetViews(*object, options.editorMode, options.nightMode) )
			{
				enumZOrder z = view.zfunc->GetZ(world, *object);
				if( Z_NONE != z && object->GetGridSet() )
					zLayers[z].emplace_back(object, view.rfunc.get());
			}
		}
	}

	FOREACH( world.GetList(LIST_gsprites), GC_MovingObject, object )
	{
		for( auto &view: _renderScheme.GetViews(*object, options.editorMode, options.nightMode) )
		{
			enumZOrder z = view.zfunc->GetZ(world, *object);
			if( Z_NONE != z && !object->GetGridSet() )
				zLayers[z].emplace_back(object, view.rfunc.get());
		}
	}


	//
	// draw world to rgb
	//

	rc.SetMode(RM_WORLD);

	_terrain.Draw(rc, world.GetBounds(), options.drawGrid);

	for( int z = 0; z < Z_COUNT; ++z )
	{
		for( auto &moWithView: zLayers[z] )
			moWithView.second->Draw(world, *moWithView.first, rc);
		zLayers[z].clear();
	}

	rc.SetMode(RM_INTERFACE);


	if (world._field && options.visualizeField)
	{
		auto bounds = world._field->GetBounds();

		int xmin = std::max(bounds.left, (int)std::floor(visibleRegion.left / WORLD_BLOCK_SIZE - 0.5f));
		int ymin = std::max(bounds.top, (int)std::floor(visibleRegion.top / WORLD_BLOCK_SIZE - 0.5f));
		int xmax = std::min(bounds.right, (int)std::floor(visibleRegion.right / WORLD_BLOCK_SIZE + 0.5f));
		int ymax = std::min(bounds.bottom, (int)std::floor(visibleRegion.bottom / WORLD_BLOCK_SIZE + 0.5f));

		for (int x = xmin; x <= xmax; ++x)
		{
			for (int y = ymin; y <= ymax; ++y)
			{
				if ((*world._field)(x, y)._prop)
				{
					rc.DrawSprite(MakeRectWH(vec2d{ (float)x - 0.5f, (float)y - 0.5f } * WORLD_BLOCK_SIZE,
					                         vec2d{ WORLD_BLOCK_SIZE , WORLD_BLOCK_SIZE }), _texField, 0xffffffff, 0);
				}
			}
		}
	}

	if (aiManager && options.visualizePath)
	{
		std::vector<const AIController*> controllers;
		aiManager->GetControllers(controllers);

		for (auto controller: controllers)
		{
			auto &path = controller->GetPath();
			for (size_t i = 1; i < path.size(); i++)
			{
				rc.DrawLine(_lineTex, 0x80408040, path[i - 1], path[i], 0);
			}
		}
	}
}

vec2d ComputeWorldTransformOffset(const FRECT &canvasViewport, vec2d eye, float zoom)
{
	vec2d eyeOffset = Vec2dFloor(eye * zoom - Size(canvasViewport) / 2);
	return Offset(canvasViewport) - eyeOffset;
}
