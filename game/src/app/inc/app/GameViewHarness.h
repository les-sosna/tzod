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

RectRB GetCameraViewport(int screenW, int screenH, unsigned int camCount, unsigned int camIndex);

class GameViewHarness
    : ObjectListener<GC_Explosion>
    , ObjectListener<GC_RigidBodyStatic>
    , ObjectListener<World>
{
public:
    GameViewHarness(World &world);
    ~GameViewHarness();

    struct CanvasToWorldResult
    {
        vec2d worldPos;
        bool visible;
    };
    
    CanvasToWorldResult CanvasToWorld(const GC_Player &player, int x, int y) const;
    void SetCanvasSize(int pxWidth, int pxHeight);
    void RenderGame(DrawingContext &dc, const WorldView &worldView, vec2d defaultEye, float defaultZoom) const;
    void Step(float dt);
    
private:
    World &_world;
    std::unordered_map<const GC_Player*, Camera> _cameras;
    int _pxWidth;
    int _pxHeight;
    
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
