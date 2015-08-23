#include "inc/app/GameViewHarness.h"
#include "inc/app/Camera.h"
#include <gc/Explosion.h>
#include <gc/Player.h>
#include <gc/Vehicle.h>
#include <gc/World.h>
#include <gc/Macros.h>
#include <render/WorldView.h>
#include <cassert>


RectRB GetCameraViewport(int screenW, int screenH, unsigned int camCount, unsigned int camIndex)
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
            if (screenW >= screenH)
            {
                viewports[0] = CRect(0,             0,             screenW/2 - 1, screenH);
                viewports[1] = CRect(screenW/2 + 1, 0,             screenW,       screenH);
            }
            else
            {
                viewports[0] = CRect(0,             0,             screenW, screenH/2 - 1);
                viewports[1] = CRect(0, screenH/2 + 1,             screenW,       screenH);
            }
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

GameViewHarness::GameViewHarness(World &world)
  : _world(world)
{
    _world.eWorld.AddListener(*this);
    _world.eGC_RigidBodyStatic.AddListener(*this);
    _world.eGC_Explosion.AddListener(*this);
    
    FOREACH(world.GetList(LIST_players), GC_Player, player)
    {
        OnNewObject(*player);
    }
}

GameViewHarness::~GameViewHarness()
{
    _world.eGC_Explosion.RemoveListener(*this);
    _world.eGC_RigidBodyStatic.RemoveListener(*this);
    _world.eWorld.RemoveListener(*this);
}

GameViewHarness::CanvasToWorldResult GameViewHarness::CanvasToWorld(const GC_Player &player, int x, int y) const
{
    assert(_cameras.count(&player));
    CanvasToWorldResult result;
    const Camera &camera = _cameras.find(&player)->second;
    RectRB vp = GetCameraViewport(_pxWidth, _pxHeight, (unsigned int) _cameras.size(), camera.GetIndex());
    result.visible = vp.left <= x && x < vp.right && vp.top <= y && y < vp.bottom;
    result.worldPos = camera.GetCameraPos() - vec2d((float) WIDTH(vp), (float)HEIGHT(vp)) / 2 + vec2d((float) x, (float) y);
    return result;
}

void GameViewHarness::SetCanvasSize(int pxWidth, int pxHeight)
{
    _pxWidth = pxWidth;
    _pxHeight = pxHeight;
}

void GameViewHarness::RenderGame(DrawingContext &dc, const WorldView &worldView,
                                 vec2d defaultEye, float defaultZoom) const
{    
    if( !_cameras.empty() )
    {
        const float zoom = 1.0f;
        size_t camCount = _cameras.size();
        if( _pxWidth >= _world._sx && _pxHeight >= _world._sy )
        {
            // render from single camera with maximum shake
            const Camera *camera = nullptr;
            for( auto &it: _cameras )
            {
                if( !camera || it.second.GetShake() > camera->GetShake() )
                {
                    camera = &it.second;
                }
            }
            assert(camera);
            
            RectRB viewport = GetCameraViewport(_pxWidth, _pxHeight, 1, 0);
            vec2d eye = camera->GetCameraPos();
            worldView.Render(dc, _world, viewport, eye, zoom, false, false, _world.GetNightMode());
        }
        else
        {
            // render from each camera
            size_t camIndex = 0;
            for( auto &it: _cameras )
            {
                RectRB viewport = GetCameraViewport(_pxWidth, _pxHeight, camCount, camIndex);
                vec2d eye = it.second.GetCameraPos();
                worldView.Render(dc, _world, viewport, eye, zoom, false, false, _world.GetNightMode());
                ++camIndex;
            }
        }
    }
    else
    {
        // render from default camera
        CRect viewport(0, 0, _pxWidth, _pxHeight);
        worldView.Render(dc, _world, viewport, defaultEye, defaultZoom, false, false, _world.GetNightMode());
    }
}

void GameViewHarness::Step(float dt)
{
    bool singleCamera = _pxWidth >= _world._sx && _pxHeight >= _world._sy;
    size_t camCount = _cameras.size();
    for( auto &it: _cameras )
    {
        RectRB viewport = GetCameraViewport(_pxWidth, _pxHeight, singleCamera ? 1 : camCount, singleCamera ? 0 : it.second.GetIndex());
        it.second.CameraTimeStep(_world, it.first->GetVehicle(), dt, vec2d((float) WIDTH(viewport), (float) HEIGHT(viewport)));
    }
}

void GameViewHarness::OnBoom(GC_Explosion &obj, float radius, float damage)
{
    for( auto &it: _cameras )
    {
        if( const GC_Vehicle *vehicle = it.first->GetVehicle() )
        {
            float level = 0.5f * (radius - (obj.GetPos() - vehicle->GetPos()).len() * 0.3f) / radius;
            if( level > 0 )
            {
                it.second.Shake(level);
            }
        }
    }
}

void GameViewHarness::OnDamage(GC_RigidBodyStatic &obj, const DamageDesc &dd)
{
    for( auto &it: _cameras )
    {
        if( &obj == it.first->GetVehicle() )
        {
            it.second.Shake(obj.GetHealth() <= 0 ? 2.0f : dd.damage / obj.GetHealthMax());
            break;
        }
    }
}

void GameViewHarness::OnKill(GC_Object &obj)
{
    ObjectType type = obj.GetType();
    if (GC_Player::GetTypeStatic() == type)
    {
        assert(_cameras.count(static_cast<const GC_Player*>(&obj)));
        _cameras.erase(static_cast<const GC_Player*>(&obj));
    }
}

void GameViewHarness::OnNewObject(GC_Object &obj)
{
    ObjectType type = obj.GetType();
    if (GC_Player::GetTypeStatic() == type)
    {
        auto &player = static_cast<const GC_Player&>(obj);
        vec2d pos = player.GetVehicle() ? player.GetVehicle()->GetPos() : vec2d(_world._sx / 2, _world._sy / 2);
        assert(!_cameras.count(&player));
        _cameras.emplace(&player, Camera(pos, (unsigned int) _cameras.size()));
    }
}
