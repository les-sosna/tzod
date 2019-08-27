#pragma once
#include "Camera.h"
#include <gc/WorldEvents.h>
#include <math/MyMath.h>
#include <stddef.h>
#include <vector>

class GC_Player;
class AIManager;
class RenderContext;
class World;
class WorldController;
class WorldView;

RectRB GetCameraViewport(int screenW, int screenH, unsigned int camCount, unsigned int camIndex);

class GameViewHarness
	: ObjectListener<GC_Explosion>
	, ObjectListener<GC_RigidBodyStatic>
{
public:
	GameViewHarness(World &world, WorldController &worldController);
	~GameViewHarness();

	struct CanvasToWorldResult
	{
		vec2d worldPos;
		bool visible;
	};

	vec2d GetListenerPos() const;
	World& GetWorld() const { return _world; }
	CanvasToWorldResult CanvasToWorld(unsigned int viewIndex, int x, int y) const;
	vec2d WorldToCanvas(unsigned int viewIndex, vec2d worldPos) const;
	void SetCanvasSize(int pxWidth, int pxHeight, float scale);
	void RenderGame(RenderContext &rc, const WorldView &worldView, bool visualizeField, const AIManager *aiManager) const;
	void Step(float dt);

private:
	World &_world;
	std::vector<Camera> _cameras;
	int _pxWidth = 0;
	int _pxHeight = 0;
	float _scale = 1;

	mutable const Camera *_maxShakeCamera = nullptr;
	const Camera& GetMaxShakeCamera() const;
	bool IsSingleCamera() const;

	// ObjectListener<Explosion>
	void OnBoom(GC_Explosion &obj, float radius, float damage) override;

	// ObjectListener<GC_RigidBodyStatic>
	void OnDestroy(GC_RigidBodyStatic &obj, const DamageDesc &dd) override {}
	void OnDamage(GC_RigidBodyStatic &obj, const DamageDesc &dd) override;
};
