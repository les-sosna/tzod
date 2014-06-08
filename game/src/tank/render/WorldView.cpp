#include "WorldView.h"
#include "Macros.h"
#include "config/Config.h"
#include "gc/Camera.h"
#include "gc/Light.h"
#include "gc/World.h"
#include "video/RenderBase.h"

WorldView::WorldView(IRender &render, TextureManager &texman)
    : _render(render)
    , _texman(texman)
    , _texBack(texman.FindSprite("background"))
    , _texGrid(texman.FindSprite("grid"))
{
}

void WorldView::Render(World &world, bool editorMode) const
{
	_render.SetAmbient(g_conf.sv_nightmode.Get() ? (editorMode ? 0.5f : 0) : 1);

	if( editorMode || world.GetList(LIST_cameras).empty() )
	{
		// render from default camera
		_render.Camera(NULL, world._defaultCamera.GetPosX(), world._defaultCamera.GetPosY(), world._defaultCamera.GetZoom(), 0);

		FRECT viewRect;
		viewRect.left = world._defaultCamera.GetPosX();
		viewRect.top = world._defaultCamera.GetPosY();
		viewRect.right = viewRect.left + (float) _render.GetWidth() / world._defaultCamera.GetZoom();
		viewRect.bottom = viewRect.top + (float) _render.GetHeight() / world._defaultCamera.GetZoom();

		RenderInternal(world, viewRect, editorMode);
	}
	else
	{
		if( _render.GetWidth() >= int(world._sx) && _render.GetHeight() >= int(world._sy) )
		{
			// render from single camera with maximum shake
			float max_shake = -1;
			GC_Camera *singleCamera = NULL;
			FOREACH( world.GetList(LIST_cameras), GC_Camera, pCamera )
			{
				if( pCamera->GetShake() > max_shake )
				{
					singleCamera = pCamera;
					max_shake = pCamera->GetShake();
				}
			}
			assert(singleCamera);

			FRECT viewRect;
			singleCamera->GetWorld(viewRect);

			Rect screen;
			singleCamera->GetScreen(screen);

			g_render->Camera(&screen,
				viewRect.left,
				viewRect.top,
				singleCamera->GetZoom(),
				g_conf.g_rotcamera.Get() ? singleCamera->GetAngle() : 0);

			RenderInternal(world, viewRect, editorMode);
		}
		else
		{
			// render from each camera
			FOREACH( world.GetList(LIST_cameras), GC_Camera, pCamera )
			{
				FRECT viewRect;
				pCamera->GetWorld(viewRect);

				Rect screen;
				pCamera->GetScreen(screen);

				g_render->Camera(&screen,
					viewRect.left,
					viewRect.top,
					pCamera->GetZoom(),
					g_conf.g_rotcamera.Get() ? pCamera->GetAngle() : 0);

				RenderInternal(world, viewRect, editorMode);
			}
		}
	}

#ifdef _DEBUG
	if (glfwGetKey(g_appWindow, GLFW_KEY_BACKSPACE) != GLFW_PRESS)
#endif
	{
		_dbgLineBuffer.clear();
	}
}

void WorldView::RenderInternal(World &world, const FRECT &view, bool editorMode) const
{
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
			if( pLight->IsActive() &&
				pLight->GetPos().x + pLight->GetRenderRadius() > xmin &&
				pLight->GetPos().x - pLight->GetRenderRadius() < xmax &&
				pLight->GetPos().y + pLight->GetRenderRadius() > ymin &&
				pLight->GetPos().y - pLight->GetRenderRadius() < ymax )
			{
				pLight->Shine(*g_render);
			}
		}
	}


	//
	// draw world to rgb
	//

	_render.SetMode(RM_WORLD);

	// background texture
	DrawBackground(world._sx, world._sy, _texBack);
	if( editorMode && g_conf.ed_drawgrid.Get() )
		DrawBackground(world._sx, world._sy, _texGrid);


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
            if( object->GetVisible() && Z_NONE != object->GetZ() && object->GetGridSet() )
                zLayers[object->GetZ()].push_back(object);
        }
    }

    FOREACH( world.GetList(LIST_gsprites), GC_2dSprite, object )
    {
        if( object->GetVisible() && Z_NONE != object->GetZ() && !object->GetGridSet() )
            zLayers[object->GetZ()].push_back(object);
    }

    for( int z = 0; z < Z_COUNT; ++z )
    {
        for( GC_2dSprite *sprite: zLayers[z] )
            sprite->Draw(static_cast<DrawingContext&>(_texman), editorMode);
        zLayers[z].clear();
    }
    
	if( !_dbgLineBuffer.empty() )
	{
		_render.DrawLines(&*_dbgLineBuffer.begin(), _dbgLineBuffer.size());
	}
}

#ifndef NDEBUG
void WorldView::DbgLine(const vec2d &v1, const vec2d &v2, SpriteColor color) const
{
	_dbgLineBuffer.push_back(MyLine());
	MyLine &line = _dbgLineBuffer.back();
	line.begin = v1;
	line.end = v2;
	line.color = color;
}
#endif

void WorldView::DrawBackground(float sizeX, float sizeY, size_t tex) const
{
	const LogicalTexture &lt = _texman.Get(tex);
	MyVertex *v = _render.DrawQuad(lt.dev_texture);
	v[0].color = 0xffffffff;
	v[0].u = 0;
	v[0].v = 0;
	v[0].x = 0;
	v[0].y = 0;
	v[1].color = 0xffffffff;
	v[1].u = sizeX / lt.pxFrameWidth;
	v[1].v = 0;
	v[1].x = sizeX;
	v[1].y = 0;
	v[2].color = 0xffffffff;
	v[2].u = sizeX / lt.pxFrameWidth;
	v[2].v = sizeY / lt.pxFrameHeight;
	v[2].x = sizeX;
	v[2].y = sizeY;
	v[3].color = 0xffffffff;
	v[3].u = 0;
	v[3].v = sizeY / lt.pxFrameHeight;
	v[3].x = 0;
	v[3].y = sizeY;
}

