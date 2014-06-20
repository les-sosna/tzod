#include "WorldView.h"
#include "rLight.h"
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
				DrawLight(*g_render, *pLight);
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
}

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

