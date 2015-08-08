#pragma once

#include "Camera.h"
#include <gc/WorldEvents.h>
#include <math/MyMath.h>
#include <stddef.h>
#include <unordered_map>

class DrawingContext;
class World;
class WorldView;
class GC_Player;

RectRB GetCameraViewport(int screenW, int screenH, size_t camCount, size_t camIndex);

class GameViewHarness
    : ObjectListener<GC_Explosion>
    , ObjectListener<GC_RigidBodyStatic>
    , ObjectListener<World>
{
public:
    GameViewHarness(World &world);
    ~GameViewHarness();
    
    void RenderGame(DrawingContext &dc, const WorldView &worldView, int width, int height,
                    vec2d defaultEye, float defaultZoom) const;
    void Step(float dt, int width, int height);
    
private:
    World &_world;
    std::unordered_map<const GC_Player*, Camera> _cameras;
    
    // ObjectListener<Explosion>
    void OnBoom(GC_Explosion &obj, float radius, float damage) override;
    
    // ObjectListener<GC_RigidBodyStatic>
    void OnDestroy(GC_RigidBodyStatic &obj, const DamageDesc &dd) override {}
    void OnDamage(GC_RigidBodyStatic &obj, const DamageDesc &dd) override;

    // ObjectListener<World>
    void OnGameStarted() override {}
    void OnGameFinished() override {}
    void OnKill(GC_Object &obj) override;
    void OnNewObject(GC_Object &obj) override;
};
