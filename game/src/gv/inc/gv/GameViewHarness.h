#pragma once

#include "Camera.h"
#include <gc/WorldEvents.h>
#include <math/MyMath.h>
#include <stddef.h>
#include <vector>

class DrawingContext;
class World;
class WorldController;
class WorldView;
class GC_Player;

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

    CanvasToWorldResult CanvasToWorld(unsigned int viewIndex, int x, int y) const;
    void SetCanvasSize(int pxWidth, int pxHeight, float scale);
    void RenderGame(DrawingContext &dc, const WorldView &worldView, vec2d defaultEye, float defaultZoom) const;
    void Step(float dt);

private:
    World &_world;
    WorldController &_worldController;
    std::vector<Camera> _cameras;
    int _pxWidth;
    int _pxHeight;
    float _scale;

    mutable const Camera *_maxShakeCamera;
    const Camera& GetMaxShakeCamera() const;
    bool IsSingleCamera() const;

    // ObjectListener<Explosion>
    void OnBoom(GC_Explosion &obj, float radius, float damage) override;

    // ObjectListener<GC_RigidBodyStatic>
    void OnDestroy(GC_RigidBodyStatic &obj, const DamageDesc &dd) override {}
    void OnDamage(GC_RigidBodyStatic &obj, const DamageDesc &dd) override;
};
