#include "inc/gv/GameViewHarness.h"
#include "inc/gv/Camera.h"
#include <ctx/WorldController.h>
#include <gc/Explosion.h>
#include <gc/Player.h>
#include <gc/Vehicle.h>
#include <gc/World.h>
#include <render/WorldView.h>
#include <video/RenderContext.h>
#include <cassert>


RectRB GetCameraViewport(int screenW, int screenH, unsigned int camCount, unsigned int camIndex)
{
	assert(camCount > 0 && camCount <= 4);
	assert(camIndex < camCount);

	RectRB viewports[4];

	switch( camCount )
	{
		case 1:
			viewports[0] = RectRB{      0,             0,            screenW,         screenH };
			break;
		case 2:
			if (screenW >= screenH)
			{
				viewports[0] = RectRB{ 0,              0,            screenW / 2 - 1, screenH };
				viewports[1] = RectRB{screenW / 2 + 1, 0,            screenW,         screenH };
			}
			else
			{
				viewports[0] = RectRB{ 0,               0,           screenW,         screenH / 2 - 1 };
				viewports[1] = RectRB{ 0, screenH / 2 + 1,           screenW,         screenH };
			}
			break;
		case 3:
			viewports[0] = RectRB{ 0,               0,               screenW / 2 - 1, screenH / 2 - 1 };
			viewports[1] = RectRB{ screenW / 2 + 1, 0,               screenW,         screenH / 2 - 1 };
			viewports[2] = RectRB{ screenW / 4,     screenH / 2 + 1, screenW * 3 / 4, screenH};
			break;
		case 4:
			viewports[0] = RectRB{ 0,               0,               screenW / 2 - 1, screenH / 2 - 1 };
			viewports[1] = RectRB{ screenW / 2 + 1, 0,               screenW,         screenH / 2 - 1 };
			viewports[2] = RectRB{ 0,               screenH / 2 + 1, screenW / 2 - 1, screenH };
			viewports[3] = RectRB{ screenW / 2 + 1, screenH / 2 + 1, screenW,         screenH };
			break;
		default:
			assert(false);
	}

	return viewports[camIndex];
}

GameViewHarness::GameViewHarness(World &world, WorldController &worldController)
  : _world(world)
  , _scale(1)
  , _maxShakeCamera(nullptr)
{
	_world.eGC_RigidBodyStatic.AddListener(*this);
	_world.eGC_Explosion.AddListener(*this);

	for (GC_Player *player: worldController.GetLocalPlayers())
	{
		assert(player);
		vec2d pos = player->GetVehicle() ? player->GetVehicle()->GetPos() : Center(_world.GetBounds());
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
	result.worldPos = camera.GetCameraPos() + (vec2d{ (float)x, (float)y } -vec2d{ (float)WIDTH(viewport), (float)HEIGHT(viewport) } / 2) / _scale;
	return result;
}

vec2d GameViewHarness::WorldToCanvas(unsigned int viewIndex, vec2d worldPos) const
{
	assert(viewIndex < _cameras.size());
	const Camera &camera = IsSingleCamera() ? GetMaxShakeCamera() : _cameras[viewIndex];
	RectRB viewport = camera.GetViewport();
	vec2d viewPos = (worldPos - camera.GetCameraPos()) * _scale + vec2d{ (float)WIDTH(viewport), (float)HEIGHT(viewport) } / 2;
	return viewPos + vec2d{ (float)viewport.left, (float)viewport.top };
}

void GameViewHarness::SetCanvasSize(int pxWidth, int pxHeight, float scale)
{
	_pxWidth = pxWidth;
	_pxHeight = pxHeight;
	_scale = scale;

	unsigned int camCount = static_cast<unsigned int>(_cameras.size());
	for (unsigned int camIndex = 0; camIndex != camCount; ++camIndex)
	{
		auto effectiveCount = IsSingleCamera() ? 1 : camCount;
		auto effectiveIndex = IsSingleCamera() ? 0 : camIndex;
		_cameras[camIndex].SetViewport(::GetCameraViewport(_pxWidth, _pxHeight, effectiveCount, effectiveIndex));
	}
}

void GameViewHarness::RenderGame(RenderContext &rc, const WorldView &worldView, bool visualizeField, const AIManager *aiManager) const
{
	WorldViewRenderOptions options;
	options.nightMode = _world.GetNightMode();
	options.visualizeField = visualizeField;
	options.visualizePath = !!aiManager;

	if( !_cameras.empty() )
	{
		if (IsSingleCamera())
		{
			vec2d eye = GetMaxShakeCamera().GetCameraPos();
			rc.PushClippingRect(GetMaxShakeCamera().GetViewport());
			rc.PushWorldTransform(ComputeWorldTransformOffset(RectToFRect(GetMaxShakeCamera().GetViewport()), eye, _scale), _scale);
			worldView.Render(rc, _world, options, aiManager);
			rc.PopTransform();
			rc.PopClippingRect();
		}
		else
		for( auto &camera: _cameras )
		{
			vec2d eye = camera.GetCameraPos();
			rc.PushClippingRect(camera.GetViewport());
			rc.PushWorldTransform(ComputeWorldTransformOffset(RectToFRect(camera.GetViewport()), eye, _scale), _scale);
			worldView.Render(rc, _world, options, aiManager);
			rc.PopTransform();
			rc.PopClippingRect();
		}
	}
	else
	{
		vec2d eye = Center(_world.GetBounds());
		float zoom = std::max(_pxWidth / WIDTH(_world.GetBounds()), _pxHeight / HEIGHT(_world.GetBounds()));

		RectRB viewport{ 0, 0, _pxWidth, _pxHeight };
		rc.PushClippingRect(viewport);
		rc.PushWorldTransform(ComputeWorldTransformOffset(RectToFRect(viewport), eye, zoom), zoom);
		worldView.Render(rc, _world, options, aiManager);
		rc.PopTransform();
		rc.PopClippingRect();
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
	for (auto &camera: _cameras)
	{
		camera.CameraTimeStep(_world, dt, _scale);
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
	return _pxWidth / _scale >= WIDTH(_world.GetBounds()) && _pxHeight / _scale >= HEIGHT(_world.GetBounds());
}

void GameViewHarness::OnBoom(GC_Explosion &obj, float radius, float damage)
{
	for( auto &camera: _cameras )
	{
		RectRB viewport = camera.GetViewport();
		vec2d viewSize{ (float)WIDTH(viewport), (float)HEIGHT(viewport) };
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
