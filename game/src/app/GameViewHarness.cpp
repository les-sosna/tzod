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
  , _maxShakeCamera(nullptr)
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
    const Camera &camera = IsSingleCamera() ? GetMaxShakeCamera() :_cameras.find(&player)->second;
    RectRB viewport = camera.GetViewport();
    x -= viewport.left;
    y -= viewport.top;
    result.visible = (0 <= x && x < viewport.right && 0 <= y && y < viewport.bottom);
    result.worldPos = camera.GetCameraPos() - vec2d((float) WIDTH(viewport), (float)HEIGHT(viewport)) / 2 + vec2d((float) x, (float) y);
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
        if (IsSingleCamera())
        {
            vec2d eye = GetMaxShakeCamera().GetCameraPos();
            worldView.Render(dc, _world, GetMaxShakeCamera().GetViewport(), eye, zoom, false, false, _world.GetNightMode());
        }
        else
        {
            // render from each camera
            for( auto &p2c: _cameras )
            {
                vec2d eye = p2c.second.GetCameraPos();
                worldView.Render(dc, _world, p2c.second.GetViewport(), eye, zoom, false, false, _world.GetNightMode());
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

static const Camera* FindMaxShakeCamera(const std::unordered_map<const GC_Player*, Camera> &cameras)
{
    const Camera *maxShakeCamera = nullptr;
    for (auto &p2c : cameras)
    {
        if (!maxShakeCamera || p2c.second.GetShake() > maxShakeCamera->GetShake())
        {
            maxShakeCamera = &p2c.second;
        }
    }
    return maxShakeCamera;
}

void GameViewHarness::Step(float dt)
{
    size_t camCount = _cameras.size();
    for( auto &p2c: _cameras )
    {
        p2c.second.SetViewport(GetCameraViewport(_pxWidth, _pxHeight, IsSingleCamera() ? 1 : camCount, IsSingleCamera() ? 0 : p2c.second.GetIndex()));
        p2c.second.CameraTimeStep(_world, p2c.first->GetVehicle(), dt);
    }
    _maxShakeCamera = nullptr;
}

const Camera& GameViewHarness::GetMaxShakeCamera() const
{
    if (!_maxShakeCamera)
    {
        _maxShakeCamera = FindMaxShakeCamera(_cameras);
        assert(_maxShakeCamera);
    }
    return *_maxShakeCamera;
}

bool GameViewHarness::IsSingleCamera() const
{
    return _pxWidth >= _world._sx && _pxHeight >= _world._sy;
}

void GameViewHarness::OnBoom(GC_Explosion &obj, float radius, float damage)
{
    for( auto &p2c: _cameras )
    {
        RectRB viewport = p2c.second.GetViewport();
        vec2d viewSize = vec2d((float)WIDTH(viewport), (float)HEIGHT(viewport));
        vec2d lt = p2c.second.GetCameraPos() - viewSize / 2;
        vec2d rb = lt + viewSize;

        float distanceLeft = lt.x - obj.GetPos().x;
        float distanceTop = lt.y - obj.GetPos().y;
        float distanceRight = obj.GetPos().x - rb.x;
        float distanceBottom = obj.GetPos().y - rb.y;
        float maxDistance = std::max(std::max(distanceLeft, distanceTop), std::max(distanceRight, distanceBottom));

        if (maxDistance < radius)
        {
            float sizeFactor = std::max(0.f, (radius - 50) / 100);
            float damageFactor = damage / 100;
            float distanceFactor = 1.0f - std::max(.0f, maxDistance) / radius;
            p2c.second.Shake(sizeFactor * damageFactor * distanceFactor);
            _maxShakeCamera = nullptr;
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
            _maxShakeCamera = nullptr;
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
        _maxShakeCamera = nullptr;
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
        _maxShakeCamera = nullptr;
    }
}
