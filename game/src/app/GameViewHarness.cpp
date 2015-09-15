#include "inc/app/GameViewHarness.h"
#include "inc/app/Camera.h"
#include "inc/app/WorldController.h"
#include <gc/Explosion.h>
#include <gc/Player.h>
#include <gc/Vehicle.h>
#include <gc/World.h>
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

GameViewHarness::GameViewHarness(World &world, WorldController &worldController)
  : _world(world)
  , _worldController(worldController)
  , _maxShakeCamera(nullptr)
{
    _world.eGC_RigidBodyStatic.AddListener(*this);
    _world.eGC_Explosion.AddListener(*this);

	for (GC_Player *player: worldController.GetLocalPlayers())
	{
		assert(player);
		vec2d pos = player->GetVehicle() ? player->GetVehicle()->GetPos() : vec2d(_world._sx / 2, _world._sy / 2);
		_cameras.emplace_back(pos, *player);
	}
}

GameViewHarness::~GameViewHarness()
{
    _world.eGC_Explosion.RemoveListener(*this);
    _world.eGC_RigidBodyStatic.RemoveListener(*this);
}

GameViewHarness::CanvasToWorldResult GameViewHarness::CanvasToWorld(unsigned int viewIndex, int x, int y) const
{
    assert(viewIndex < _cameras.size());
    CanvasToWorldResult result;
    const Camera &camera = IsSingleCamera() ? GetMaxShakeCamera() :_cameras[viewIndex];
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
        else for( auto &camera: _cameras )
        {
            vec2d eye = camera.GetCameraPos();
            worldView.Render(dc, _world, camera.GetViewport(), eye, zoom, false, false, _world.GetNightMode());
        }
    }
    else
    {
        // render from default camera
        CRect viewport(0, 0, _pxWidth, _pxHeight);
        worldView.Render(dc, _world, viewport, defaultEye, defaultZoom, false, false, _world.GetNightMode());
    }
}

static const Camera* FindMaxShakeCamera(const std::vector<Camera> &cameras)
{
    const Camera *maxShakeCamera = nullptr;
    for (auto &camera : cameras)
    {
        if (!maxShakeCamera || camera.GetShake() > maxShakeCamera->GetShake())
        {
            maxShakeCamera = &camera;
        }
    }
    return maxShakeCamera;
}

void GameViewHarness::Step(float dt)
{
    size_t camCount = _cameras.size();
	for (unsigned int i = 0; i != camCount; ++i)
    {
		auto &camera = _cameras[i];
        camera.SetViewport(GetCameraViewport(_pxWidth, _pxHeight, IsSingleCamera() ? 1 : camCount, IsSingleCamera() ? 0 : i));
        camera.CameraTimeStep(_world, dt);
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
    for( auto &camera: _cameras )
    {
        RectRB viewport = camera.GetViewport();
        vec2d viewSize = vec2d((float)WIDTH(viewport), (float)HEIGHT(viewport));
        vec2d lt = camera.GetCameraPos() - viewSize / 2;
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
            camera.Shake(sizeFactor * damageFactor * distanceFactor);
            _maxShakeCamera = nullptr;
        }
    }
}

void GameViewHarness::OnDamage(GC_RigidBodyStatic &obj, const DamageDesc &dd)
{
    for( auto &camera: _cameras )
    {
        if( &obj == camera.GetPlayer().GetVehicle() )
        {
            camera.Shake(obj.GetHealth() <= 0 ? 2.0f : dd.damage / obj.GetHealthMax());
            _maxShakeCamera = nullptr;
            break;
        }
    }
}
