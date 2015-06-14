#include "GameView.h"
#include "DefaultCamera.h"
#include <gc/Camera.h>
#include <gc/Macros.h>
#include <gc/World.h>
#include "render/WorldView.h"
#include <cassert>


RectRB GetCameraViewport(int screenW, int screenH, size_t camCount, size_t camIndex)
{
    assert(camCount > 0 && camCount <= 4);
    assert(camIndex < camCount);
    
    RectRB viewports[4];
    
    switch( camCount )
    {
        case 1:
            viewports[0] = CRect(0,             0,             screenW,       screenH);
            break;
        case 2:
            viewports[0] = CRect(0,             0,             screenW/2 - 1, screenH);
            viewports[1] = CRect(screenW/2 + 1, 0,             screenW,       screenH);
            break;
        case 3:
            viewports[0] = CRect(0,             0,             screenW/2 - 1, screenH/2 - 1);
            viewports[1] = CRect(screenW/2 + 1, 0,             screenW,       screenH/2 - 1);
            viewports[2] = CRect(screenW/4,     screenH/2 + 1, screenW*3/4,   screenH);
            break;
        case 4:
            viewports[0] = CRect(0,             0,             screenW/2 - 1, screenH/2 - 1);
            viewports[1] = CRect(screenW/2 + 1, 0,             screenW,       screenH/2 - 1);
            viewports[2] = CRect(0,             screenH/2 + 1, screenW/2 - 1, screenH);
            viewports[3] = CRect(screenW/2 + 1, screenH/2 + 1, screenW,       screenH);
            break;
        default:
            assert(false);
    }
    
    return viewports[camIndex];
}

void RenderGame(DrawingContext &dc, const World &world, const WorldView &worldView,
                int width, int height, const DefaultCamera &defaultCamera)
{    
    if( size_t camCount = world.GetList(LIST_cameras).size() )
    {
        if( width >= world._sx && height >= world._sy )
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
            
            RectRB viewport = CRect((width - (int) world._sx) / 2, (height - (int) world._sy) / 2,
                                    (width + (int) world._sx) / 2, (height + (int) world._sy) / 2);
            vec2d eye = singleCamera->GetCameraPos();
            float zoom = singleCamera->GetZoom();
            worldView.Render(dc, world, viewport, eye, zoom, false, false, world.GetNightMode());
        }
        else
        {
            // render from each camera
            size_t camIndex = 0;
            FOREACH( world.GetList(LIST_cameras), GC_Camera, pCamera )
            {
                RectRB viewport = GetCameraViewport(width, height, camCount, camIndex);
                vec2d eye = pCamera->GetCameraPos();
                float zoom = pCamera->GetZoom();
                worldView.Render(dc, world, viewport, eye, zoom, false, false, world.GetNightMode());
                ++camIndex;
            }
        }
    }
    else
    {
        // render from default camera
        CRect viewport(0, 0, width, height);
        vec2d eye(defaultCamera.GetPos().x + width / 2, defaultCamera.GetPos().y + height / 2);
        float zoom = defaultCamera.GetZoom();
        worldView.Render(dc, world, viewport, eye, zoom, false, false, world.GetNightMode());
    }
}
