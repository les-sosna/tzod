#pragma once
#include "AppStateListener.h"
#include <memory>

class DrawingContext;
class GameViewHarness;
class WorldView;

class GameView : AppStateListener
{
public:
    GameView(AppState &appState);
    ~GameView();

    void SetCanvasSize(int pxWidth, int pxHeight);
    void Step(float dt);
    void Render(DrawingContext &drawingContext, const WorldView &worldView);

private:
    std::unique_ptr<GameViewHarness> _harness;
    int _pxWidth;
    int _pxHeight;

    // AppStateListener
    void OnGameContextChanging() override;
    void OnGameContextChanged() override;
};
